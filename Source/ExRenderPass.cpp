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

#include <GL/glew.h>
#include <iostream>

// Shader Stage IDs
// ---------------------

enum ShaderID
{
    MESH_VS,
    UNLIT_PS,
};

// Resources
// ---------------------

static std::unordered_map<ShaderID, Shader> s_Shaders;

ExRenderPass::ExRenderPass(HdRenderIndex *index, HdRprimCollection const &collection, ExRenderDelegate* renderDelegate) 
    : HdRenderPass(index, collection), m_Owner(renderDelegate)
{
    auto device = m_Owner->GetGraphicsDevice();

    s_Shaders = 
    {
        { ShaderID::MESH_VS,  Shader("assets/TriangleVert.spv", VK_SHADER_STAGE_VERTEX_BIT)   },
        { ShaderID::UNLIT_PS, Shader("assets/TriangleFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT) }
    };

    for (auto& shader : s_Shaders)
        device->CreateShaders ({ &shader.second });
}

ExRenderPass::~ExRenderPass() {}

void ExRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    // Grab a handle to the device. 
    Device* device = m_Owner->GetGraphicsDevice();

    // grab a handle to the current frame. 
    Frame* frame = m_Owner->GetRenderSetting(TfToken("CurrentFrame")).UncheckedGet<Frame*>();

    Image::TransferBackbufferToWrite(frame->commandBuffer, device, frame->backBuffer);

    VkRenderingAttachmentInfoKHR colorAttachment
    {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView   = frame->backBufferView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color = { 0, 1, 0, 1 }
    };

    VkRenderingInfoKHR renderInfo
    {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea           = device->GetWindow()->GetScissor(),
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr
    };

    // Write commands for this frame. 
    Device::vkCmdBeginRenderingKHR(frame->commandBuffer, &renderInfo);

    Shader::Bind(frame->commandBuffer, s_Shaders[ShaderID::MESH_VS]);
    Shader::Bind(frame->commandBuffer, s_Shaders[ShaderID::UNLIT_PS]);
    
    // Configure render state
    Device::SetDefaultRenderState(frame->commandBuffer);

    auto viewport = device->GetWindow()->GetViewport();
    Device::vkCmdSetViewportWithCountEXT(frame->commandBuffer, 1u, &viewport);

    auto scissor = device->GetWindow()->GetScissor();
    Device::vkCmdSetScissorWithCountEXT(frame->commandBuffer, 1u, &scissor);

    vkCmdDraw(frame->commandBuffer, 3u, 1u, 0u, 0u);

    Device::vkCmdEndRenderingKHR(frame->commandBuffer);

    Image::TransferBackbufferToPresent(frame->commandBuffer, device, frame->backBuffer);

    // USD-View will already have a target bound in GL, just clear it for now. 
    // glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
}