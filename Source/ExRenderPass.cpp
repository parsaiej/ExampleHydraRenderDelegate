#include <ExampleDelegate/ExRenderPass.h>
#include <ExampleDelegate/ExRenderDelegate.h>

#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Shader.h>
using namespace VulkanWrappers;

#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolvedPath.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/imaging/hd/camera.h>

#include <GL/glew.h>
#include <iostream>

// Resource IDs
// ---------------------

enum ShaderID
{
    MESH_VS,
    UNLIT_PS,
};

enum ImageID
{
    COLOR,
    DEPTH
};

// Resources
// ---------------------

static VkCommandBuffer s_InternalCmd;

static std::unordered_map<ShaderID, Shader> s_Shaders;
static std::unordered_map<ImageID,  Image>  s_Images;

static unsigned int s_GLBackbufferImage;
static unsigned int s_GLBackbufferObject;

ExRenderPass::ExRenderPass(HdRenderIndex *index, HdRprimCollection const &collection, ExRenderDelegate* renderDelegate) 
    : HdRenderPass(index, collection), m_Owner(renderDelegate)
{
    auto device = m_Owner->GetGraphicsDevice();

    // Fetch the base plugin in order to construct asset paths.
    auto pluginBase = PlugRegistry::GetInstance().GetPluginWithName("hdExample");
    TF_VERIFY(pluginBase != nullptr);

    s_Shaders = 
    {
        { ShaderID::MESH_VS,  Shader(pluginBase->FindPluginResource("shaders/TriangleVert.spv").c_str(), VK_SHADER_STAGE_VERTEX_BIT)   },
        { ShaderID::UNLIT_PS, Shader(pluginBase->FindPluginResource("shaders/TriangleFrag.spv").c_str(), VK_SHADER_STAGE_FRAGMENT_BIT) }
    };

    for (auto& shader : s_Shaders)
        device->CreateShaders ({ &shader.second });

    VkFormat colorFormat;
    {
        if (device->GetWindow() != nullptr)
            colorFormat = device->GetWindow()->GetColorSurfaceFormat();
        else
            colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    }

    s_Images = 
    {
        { ImageID::COLOR, Image(1196, 756, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_ASPECT_COLOR_BIT) },
        { ImageID::DEPTH, Image(800, 600, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT) }
    };

    // Need to enable mapping in case we don't own the backbuffer.
    s_Images[ImageID::COLOR].SetMappingEnabled(m_Owner->RequiresManualQueueSubmit());

    for (auto& image : s_Images)
        device->CreateImages( { &image.second } );

    // The internal command should be secondary since there already exist a primary one to be used for application-managed queue submission.
    if (m_Owner->RequiresManualQueueSubmit())
        device->CreateCommandBuffer(&s_InternalCmd);

    // Create necesarry backbuffers if needed 
    if (m_Owner->RequiresManualQueueSubmit())
    {
        glewInit();

        GLint currentFBO = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

        char imageData[1196 * 756 * 4];
        glGenTextures(1, &s_GLBackbufferImage);
        glBindTexture(GL_TEXTURE_2D, s_GLBackbufferImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1196, 756, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

        glGenFramebuffers(1, &s_GLBackbufferObject);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GLBackbufferObject);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_GLBackbufferImage, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentFBO);
        glBindTexture(GL_TEXTURE_2D, currentFBO);
    }
}

ExRenderPass::~ExRenderPass() 
{
    auto device = m_Owner->GetGraphicsDevice();

    vkDeviceWaitIdle(device->GetLogical());

    for (auto& shader : s_Shaders)
        device->ReleaseShaders ({ &shader.second });

    for (auto& image : s_Images)
        device->ReleaseImages ({ &image.second });
}

static void GetViewportScissor(HdRenderPassStateSharedPtr const& renderPassState, VkRect2D* scissor, VkViewport* viewport)
{
    const CameraUtilFraming &framing = renderPassState->GetFraming();

    GfRect2i dataWindow;

    if (framing.IsValid())
        dataWindow = framing.dataWindow;
    else 
    {
        // For applications that use the old viewport API instead of
        // the new camera framing API.
        const GfVec4f vp = renderPassState->GetViewport();
        dataWindow = GfRect2i(GfVec2i(0), int(vp[2]), int(vp[3]));        
    }

    *viewport =
    {
        .x = 0.0,
        .y = 0.0,
        .width  = (float)dataWindow.GetWidth(),
        .height = (float)dataWindow.GetHeight(),
        .minDepth = 0.0,
        .maxDepth = 1.0
    };

    *scissor  = 
    {
        .offset.x = 0,
        .offset.y = 0,
        .extent.width  = (uint32_t)dataWindow.GetWidth(),
        .extent.height = (uint32_t)dataWindow.GetHeight()
    };
}

void ExRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    // Grab a handle to the device. 
    Device* device = m_Owner->GetGraphicsDevice();

    // grab a handle to the current frame. 
    Frame* frame = m_Owner->GetRenderSetting(TfToken("CurrentFrame")).UncheckedGet<Frame*>();

    VkRect2D   currentScissor;
    VkViewport currentViewport;
    GetViewportScissor(renderPassState, &currentScissor, &currentViewport);

    VkCommandBuffer cmd;
    {
        if (m_Owner->RequiresManualQueueSubmit())
        {
            assert(s_InternalCmd != nullptr);

            // Reset the command buffer for this frame.
            vkResetCommandBuffer(s_InternalCmd, 0x0);

            // Enable the command buffer into a recording state. 
            VkCommandBufferBeginInfo commandBegin
            {
                .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags            = 0,
                .pInheritanceInfo = nullptr
            };
            
            vkBeginCommandBuffer(s_InternalCmd, &commandBegin);

            cmd = s_InternalCmd;
        }
        else
        {
            assert(frame != nullptr);

            // A frame command buffer will be provided in a reset + record-ready state.
            cmd = frame->commandBuffer;
        }
    }

    Image::TransferUnknownToWrite(cmd, s_Images[ImageID::COLOR].GetData()->image);

    VkRenderingAttachmentInfoKHR colorAttachment
    {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView   = s_Images[ImageID::COLOR].GetData()->view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color = { 0, 1, 0, 1 }
    };

    VkRenderingInfoKHR renderInfo
    {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = currentScissor,
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr
    };

    // Write commands for this frame. 
    Device::vkCmdBeginRenderingKHR(cmd, &renderInfo);

    Shader::Bind(cmd, s_Shaders[ShaderID::MESH_VS]);
    Shader::Bind(cmd, s_Shaders[ShaderID::UNLIT_PS]);

    // Configure render state
    Device::SetDefaultRenderState(cmd);

    Device::vkCmdSetViewportWithCountEXT(cmd, 1u, &currentViewport);
    Device::vkCmdSetScissorWithCountEXT(cmd, 1u,  &currentScissor);

    vkCmdDraw(cmd, 3u, 1u, 0u, 0u);

    Device::vkCmdEndRenderingKHR(cmd);

    // Conclude internal command buffer recording.
    if (m_Owner->RequiresManualQueueSubmit())
    {
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo
        {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount   = 1u,
            .pCommandBuffers      = &cmd,
        };

        // Submit the the internal command to graphics queue.
        vkQueueSubmit(device->GetGraphicsQueue(), 1u, &submitInfo, nullptr);

        // Wait until work is done.
        vkDeviceWaitIdle(device->GetLogical());

        // Currently Hydra does not really make it easy to share memory on the device-side.
        // So we need to have a round trip copy via the CPU to the current GL backbuffer.

        /*
        void* pColorData;
        vmaMapMemory(device->GetAllocator(), s_Images[ImageID::COLOR].GetData()->allocation, &pColorData);

        // Copy the mapped data into the internal backbuffer image data. 
        glBindTexture(GL_TEXTURE_2D, s_GLBackbufferImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, currentViewport.width, currentViewport.height, 0, GL_RGBA8, GL_UNSIGNED_INT, pColorData);

        vmaUnmapMemory(device->GetAllocator(), s_Images[ImageID::COLOR].GetData()->allocation);
        */

        GLint currentFramebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);

        // Bind our internal FBO for write.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_GLBackbufferObject);

        void* pColorData;
        vmaMapMemory(device->GetAllocator(), s_Images[ImageID::COLOR].GetData()->allocation, &pColorData);

        // Copy the mapped data into the internal backbuffer image data. 
        glBindTexture(GL_TEXTURE_2D, s_GLBackbufferImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, currentViewport.width, currentViewport.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pColorData);

        vmaUnmapMemory(device->GetAllocator(), s_Images[ImageID::COLOR].GetData()->allocation);

        // Blit the internal backbuffer into the host one.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GLBackbufferObject);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFramebuffer);

        glBlitFramebuffer(0, 0, currentViewport.width, currentViewport.height, 
                          0, 0, currentViewport.width, currentViewport.height, 
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFramebuffer);
    }
    else
    {
        // Otherwise we can copy the image memory directly to the back buffer. 

        Image::TransferWriteToSource(cmd, s_Images[ImageID::COLOR].GetData()->image);
        Image::TransferUnknownToDestination(cmd, frame->backBuffer);

        VkImageCopy copyRegion =
        {
            .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .srcSubresource.layerCount = 1,
            .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .dstSubresource.layerCount = 1,
            .extent.width              = (uint32_t)currentScissor.extent.width,
            .extent.height             = (uint32_t)currentScissor.extent.height,
            .extent.depth              = 1
        };

        // Else just copy the color target into the frame-provided backbuffer.
        vkCmdCopyImage(cmd, 
                    s_Images[ImageID::COLOR].GetData()->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                    frame->backBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                    1u, &copyRegion);

        // After copying we prepare the backbuffer for swapchain present.
        Image::TransferDestinationToPresent(cmd, frame->backBuffer);
    }
}