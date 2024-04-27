#include <ExampleDelegate/ExRenderPass.h>
#include <ExampleDelegate/ExRenderDelegate.h>
#include <ExampleDelegate/StbUsage.h>

#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Buffer.h>
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

enum BufferID
{
    COLOR_MAP_STAGING,
};

// Resources
// ---------------------

static VkCommandBuffer s_InternalCmd;
static VkViewport      s_PreviousViewport;

static std::unordered_map<ShaderID, Shader> s_Shaders;
static std::unordered_map<ImageID,  Image>  s_Images;
static std::unordered_map<BufferID, Buffer> s_Buffers;

static unsigned int s_GLBackbufferImage;
static unsigned int s_GLBackbufferObject;

void CreateViewportSizedResources(Device* device, VkViewport viewport)
{
    VkFormat colorFormat;
    {
        if (device->GetWindow() != nullptr)
            colorFormat = device->GetWindow()->GetColorSurfaceFormat();
        else
            colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
    }

    s_Images[ImageID::COLOR] = Image(viewport.width, viewport.height, 
                                    colorFormat, 
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                                    VK_IMAGE_ASPECT_COLOR_BIT);

    s_Buffers[BufferID::COLOR_MAP_STAGING] = Buffer(4 * viewport.width * viewport.height, 
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, 
                                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    for (auto& image : s_Images)
        device->CreateImages( { &image.second } );

    for (auto& buffer : s_Buffers)
        device->CreateBuffers( { &buffer.second } );
}

ExRenderPass::ExRenderPass(HdRenderIndex *index, HdRprimCollection const &collection, ExRenderDelegate* renderDelegate) 
    : HdRenderPass(index, collection), m_Owner(renderDelegate)
{
    auto device = m_Owner->GetGraphicsDevice();

    // Fetch the base plugin in order to construct asset paths.
    auto pluginBase = PlugRegistry::GetInstance().GetPluginWithName("hdExample");
    TF_VERIFY(pluginBase != nullptr);

    /*
    s_Shaders = 
    {
        { ShaderID::MESH_VS,  Shader(pluginBase->FindPluginResource("shaders/TriangleVert.spv").c_str(), VK_SHADER_STAGE_VERTEX_BIT)   },
        { ShaderID::UNLIT_PS, Shader(pluginBase->FindPluginResource("shaders/TriangleFrag.spv").c_str(), VK_SHADER_STAGE_FRAGMENT_BIT) }
    };

    for (auto& shader : s_Shaders)
        device->CreateShaders ({ &shader.second });

    // The internal command should be secondary since there already exist a primary one to be used for application-managed queue submission.
    if (m_Owner->RequiresManualQueueSubmit())
        device->CreateCommandBuffer(&s_InternalCmd);
    */
}

ExRenderPass::~ExRenderPass() 
{
    auto device = m_Owner->GetGraphicsDevice();

    vkDeviceWaitIdle(device->GetLogical());

    for (auto& shader : s_Shaders)
        device->ReleaseShaders ({ &shader.second });

    for (auto& image : s_Images)
        device->ReleaseImages ({ &image.second });

    for (auto& buffer : s_Buffers)
        device->ReleaseBuffers ({ &buffer.second });
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

    if (dataWindow.GetWidth() <= 1 && dataWindow.GetHeight() <= 1)
    {
        struct { GLint x, y, w, h; } viewport;
        glGetIntegerv(GL_VIEWPORT, (GLint*)&viewport);
        
        // Final attempt if the viewport is still invalid, fetch it from GL (necesarry for Houdini).
        dataWindow = GfRect2i(GfVec2i(0), int(viewport.w), int(viewport.h));
    }
    
    viewport->x = 0.0;
    viewport->y = 0.0;
    viewport->width  = (float)dataWindow.GetWidth();
    viewport->height = (float)dataWindow.GetHeight();
    viewport->minDepth = 0.0;
    viewport->maxDepth = 1.0;

    
    scissor->offset.x = 0;
    scissor->offset.y = 0;
    scissor->extent.width  = (uint32_t)dataWindow.GetWidth();
    scissor->extent.height = (uint32_t)dataWindow.GetHeight();
    
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

    if (s_PreviousViewport.width  != currentViewport.width ||
        s_PreviousViewport.height != currentViewport.height)
    {
        CreateViewportSizedResources(device, currentViewport);
        s_PreviousViewport = currentViewport;
    }

    // Create necesarry backbuffers if needed 
    static bool bCreatedGLObjects = false;

    if (m_Owner->RequiresManualQueueSubmit() && !bCreatedGLObjects)
    {
        glewInit();

        GLint currentFramebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);

        std::vector<uint8_t> imageData(4 * currentViewport.width * currentViewport.height);
        glGenTextures(1, &s_GLBackbufferImage);
        glBindTexture(GL_TEXTURE_2D, s_GLBackbufferImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, currentViewport.width, currentViewport.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());

        glGenFramebuffers(1, &s_GLBackbufferObject);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GLBackbufferObject);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_GLBackbufferImage, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentFramebuffer);
        glBindTexture(GL_TEXTURE_2D, currentFramebuffer);

        bCreatedGLObjects = true;
    }

    VkCommandBuffer cmd;
    {
        if (m_Owner->RequiresManualQueueSubmit())
        {
            assert(s_InternalCmd != nullptr);

            // Reset the command buffer for this frame.
            vkResetCommandBuffer(s_InternalCmd, 0x0);

            // Enable the command buffer into a recording state. 
            VkCommandBufferBeginInfo commandBegin = {};
            
            commandBegin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBegin.flags            = 0;
            commandBegin.pInheritanceInfo = nullptr;
                        
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

    VkRenderingAttachmentInfoKHR colorAttachment = {};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.imageView   = s_Images[ImageID::COLOR].GetData()->view;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { 1, 0, 0, 1 };


    VkRenderingInfoKHR renderInfo = {};
    renderInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderInfo.renderArea           = currentScissor;
    renderInfo.layerCount           = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments    = &colorAttachment;
    renderInfo.pDepthAttachment     = nullptr;
    renderInfo.pStencilAttachment   = nullptr;


    // Write commands for this frame. 
#if __APPLE__
    Device::vkCmdBeginRenderingKHR(cmd, &renderInfo);
#else
    vkCmdBeginRendering(cmd, &renderInfo);
#endif

    /*
    Shader::Bind(cmd, s_Shaders[ShaderID::MESH_VS]);
    Shader::Bind(cmd, s_Shaders[ShaderID::UNLIT_PS]);

    // Configure render state
    Device::SetDefaultRenderState(cmd);

    if (m_Owner->RequiresManualQueueSubmit())
    {
        VkViewport flippedViewport = currentViewport;
        {
            flippedViewport.height = -currentViewport.height;
            flippedViewport.y = currentViewport.height;
        }

        Device::vkCmdSetViewportWithCountEXT(cmd, 1u, &flippedViewport);
        Device::vkCmdSetFrontFaceEXT(cmd, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    }
    else
        Device::vkCmdSetViewportWithCountEXT(cmd, 1u, &currentViewport);


    Device::vkCmdSetScissorWithCountEXT(cmd, 1u,  &currentScissor);

    vkCmdDraw(cmd, 3u, 1u, 0u, 0u);
    */

#if __APPLE__
    Device::vkCmdEndRenderingKHR(cmd);
#else
    vkCmdEndRendering(cmd);
#endif

    // Conclude internal command buffer recording.
    if (m_Owner->RequiresManualQueueSubmit())
    {
        // Prepare internal color target for copy.
        Image::TransferWriteToSource(cmd, s_Images[ImageID::COLOR].GetData()->image);

        // Transfer the internal color target to staging buffer memory that will be mapped after the command is executed.
        Buffer::CopyImage(cmd, &s_Images[ImageID::COLOR], &s_Buffers[BufferID::COLOR_MAP_STAGING]);

        // Conclude internal command rendering.
        vkEndCommandBuffer(cmd);
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount   = 1u;
        submitInfo.pCommandBuffers      = &cmd;
        

        // Submit the the internal command to graphics queue.
        vkQueueSubmit(device->GetGraphicsQueue(), 1u, &submitInfo, nullptr);

        // Wait until the work is done.
        vkDeviceWaitIdle(device->GetLogical());

        // Currently Hydra does not really make it easy to share memory on the device-side.
        // So we need to have a round trip copy via the CPU to the current GL backbuffer.

        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(device->GetAllocator(), s_Buffers[BufferID::COLOR_MAP_STAGING].GetData()->allocation, &allocInfo);

        auto mappedData = std::vector<uint8_t>(allocInfo.size);
        vmaCopyAllocationToMemory(device->GetAllocator(), s_Buffers[BufferID::COLOR_MAP_STAGING].GetData()->allocation, 0u, mappedData.data(), allocInfo.size);

    #if 0
        // Copy the mapped data into the internal backbuffer image data. 
        WritePNG("/Users/johnparsaie/Development/test.png", currentViewport.width, currentViewport.height, 4u, mappedData.data(), 4u);
    #endif

        GLint currentFramebuffer;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);

        // Bind our internal FBO for write.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_GLBackbufferObject);

        // Copy the mapped data into the internal backbuffer image data. 
        glBindTexture(GL_TEXTURE_2D, s_GLBackbufferImage);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, currentViewport.width, currentViewport.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mappedData.data());

        // Blit the internal backbuffer into the host one.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, s_GLBackbufferObject);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFramebuffer);

        glBlitFramebuffer(0, 0, currentViewport.width, currentViewport.height, 
                          0, 0, currentViewport.width, currentViewport.height, 
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, currentFramebuffer);
    }
    else
    {
        // Otherwise we can copy the image memory directly to the back buffer. 

        Image::TransferWriteToSource(cmd, s_Images[ImageID::COLOR].GetData()->image);
        Image::TransferUnknownToDestination(cmd, frame->backBuffer);

        VkImageCopy copyRegion = {};
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.extent.width              = (uint32_t)currentScissor.extent.width;
        copyRegion.extent.height             = (uint32_t)currentScissor.extent.height;
        copyRegion.extent.depth              = 1;
        
        // Else just copy the color target into the frame-provided backbuffer.
        vkCmdCopyImage(cmd, 
                    s_Images[ImageID::COLOR].GetData()->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                    frame->backBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                    1u, &copyRegion);

        // After copying we prepare the backbuffer for swapchain present.
        Image::TransferDestinationToPresent(cmd, frame->backBuffer);
    }
}