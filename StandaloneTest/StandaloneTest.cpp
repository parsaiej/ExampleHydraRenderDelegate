#include <iostream>
#include <memory>
#include <unordered_map>

#ifdef __APPLE__
    // MacOS fix for bug inside USD. 
    #define unary_function __unary_function
#endif

#include <pxr/pxr.h>
#include <pxr/base/tf/errorMark.h>
#include <pxr/base/gf/matrix4f.h>

// Hydra Core
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/unitTestDelegate.h>

// HDX (Hydra Utilities)
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/imaging/hdx/taskController.h>

// USD Hydra Scene Delegate Implementation.
#include <pxr/usdImaging/usdImaging/delegate.h>

// NRI: core & common extensions
#include <NRI/Include/NRI.h>
#include <NRI/Include/Extensions/NRIDeviceCreation.h>
#include <NRI/Include/Extensions/NRIHelper.h>
#include <NRI/Include/Extensions/NRIStreamer.h>
#include <NRI/Include/Extensions/NRISwapChain.h>

#if defined _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
#elif defined __linux__
    #define GLFW_EXPOSE_NATIVE_X11
#elif defined __APPLE__
    #define GLFW_EXPOSE_NATIVE_COCOA
    #include <MetalUtility/MetalUtility.h>
#else
    #error "Unknown platform"
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

PXR_NAMESPACE_USING_DIRECTIVE

// Specify our NRI usage.

struct NRIInterface
    : public nri::CoreInterface
    , public nri::HelperInterface
    , public nri::StreamerInterface
    , public nri::SwapChainInterface
{};

#define ABORT_ON_FAIL(result) \
    if ((result) != nri::Result::SUCCESS) \
        exit(1);

#define ABORT_ON_FALSE(result) \
    if (!(result)) \
        exit(1);

#define NUM_SWAP_CHAIN_IMAGE 2u
#define NUM_FRAME_IN_FLIGHT  2u

// Swapchain backbuffer image. 
struct BackBuffer
{
    nri::Descriptor* colorAttachment;
    nri::Texture*    resource;
};

// Per-frame state. 
struct Frame
{
    nri::CommandAllocator* commandAllocator;
    nri::CommandBuffer*    commandBuffer;
    nri::Descriptor*       constantBufferView;
    nri::DescriptorSet*    constantBufferDescriptorSet;
    uint64_t               constantBufferViewOffset;
};

// RHI
static NRIInterface                    NRI;
static nri::AdapterDesc                m_Adapter;
static nri::Device*                    m_Device;
static nri::Streamer*                  m_Streamer;
static nri::MemoryAllocatorInterface   m_MemoryAllocator;
static nri::CommandQueue*              m_GraphicsQueue;
static nri::Fence*                     m_FrameFence;
static nri::SwapChain*                 m_SwapChain;
static std::vector<BackBuffer>         m_SwapChainBuffers;
std::array<Frame, NUM_FRAME_IN_FLIGHT> m_Frames = {};
static nri::Window                     m_NRIWindow;
static GLFWwindow*                     m_GLFWWindow;

void InitializeDevice()
{
    // Get device adapter. 
    uint32_t adapterDescsNum = 1;
    ABORT_ON_FAIL(nri::nriEnumerateAdapters(&m_Adapter, adapterDescsNum));

#ifdef DEBUG
    bool validationLayers = true;
#else
    bool validationLayers = false;
#endif

    // Create device.
    nri::DeviceCreationDesc deviceCreationDesc = {};
#ifdef __APPLE__
    deviceCreationDesc.graphicsAPI = nri::GraphicsAPI::VULKAN;
#else
    deviceCreationDesc.graphicsAPI = nri::GraphicsAPI::D3D11;
#endif
    deviceCreationDesc.enableAPIValidation               = validationLayers;
    deviceCreationDesc.enableNRIValidation               = validationLayers;
    deviceCreationDesc.enableD3D11CommandBufferEmulation = true;
    deviceCreationDesc.spirvBindingOffsets               = { 100, 200, 300, 400 };;
    deviceCreationDesc.adapterDesc                       = &m_Adapter;
    deviceCreationDesc.memoryAllocatorInterface          = m_MemoryAllocator;
    ABORT_ON_FAIL(nri::nriCreateDevice(deviceCreationDesc, m_Device));

    // Fetch interface
    ABORT_ON_FAIL( nri::nriGetInterface(*m_Device, NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*)&NRI) );
    ABORT_ON_FAIL( nri::nriGetInterface(*m_Device, NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&NRI) );
    ABORT_ON_FAIL( nri::nriGetInterface(*m_Device, NRI_INTERFACE(nri::StreamerInterface), (nri::StreamerInterface*)&NRI) );
    ABORT_ON_FAIL( nri::nriGetInterface(*m_Device, NRI_INTERFACE(nri::SwapChainInterface), (nri::SwapChainInterface*)&NRI) );

    // Create streamer
    nri::StreamerDesc streamerDesc = {};
    streamerDesc.dynamicBufferMemoryLocation = nri::MemoryLocation::HOST_UPLOAD;
    streamerDesc.dynamicBufferUsageBits      = nri::BufferUsageBits::VERTEX_BUFFER | nri::BufferUsageBits::INDEX_BUFFER;
    streamerDesc.frameInFlightNum            = NUM_FRAME_IN_FLIGHT;
    ABORT_ON_FAIL(NRI.CreateStreamer(*m_Device, streamerDesc, m_Streamer));

    // Command queue
    ABORT_ON_FAIL(NRI.GetCommandQueue(*m_Device, nri::CommandQueueType::GRAPHICS, m_GraphicsQueue));

    // Fences
    ABORT_ON_FAIL(NRI.CreateFence(*m_Device, 0, m_FrameFence));

    // Swap chain
    nri::Format swapChainFormat;
    {
        nri::SwapChainDesc swapChainDesc = {};
        swapChainDesc.window               = m_NRIWindow;
        swapChainDesc.commandQueue         = m_GraphicsQueue;
        swapChainDesc.format               = nri::SwapChainFormat::BT709_G22_8BIT;
        swapChainDesc.verticalSyncInterval = 0u;
        swapChainDesc.width                = (uint16_t)800;
        swapChainDesc.height               = (uint16_t)600;
        swapChainDesc.textureNum           = NUM_SWAP_CHAIN_IMAGE;
        ABORT_ON_FAIL( NRI.CreateSwapChain(*m_Device, swapChainDesc, m_SwapChain) );

        uint32_t swapChainTextureNum;
        nri::Texture* const* swapChainTextures = NRI.GetSwapChainTextures(*m_SwapChain, swapChainTextureNum);
        swapChainFormat = NRI.GetTextureDesc(*swapChainTextures[0]).format;

        for (uint32_t i = 0; i < swapChainTextureNum; i++)
        {
            nri::Texture2DViewDesc textureViewDesc = {swapChainTextures[i], nri::Texture2DViewType::COLOR_ATTACHMENT, swapChainFormat};

            nri::Descriptor* colorAttachment;
            ABORT_ON_FAIL( NRI.CreateTexture2DView(textureViewDesc, colorAttachment) );

            const BackBuffer backBuffer = { colorAttachment, swapChainTextures[i] };
            m_SwapChainBuffers.push_back(backBuffer);
        }
    }

    // Buffered resources
    for (Frame& frame : m_Frames)
    {
        ABORT_ON_FAIL( NRI.CreateCommandAllocator(*m_GraphicsQueue, frame.commandAllocator) );
        ABORT_ON_FAIL( NRI.CreateCommandBuffer(*frame.commandAllocator, frame.commandBuffer) );
    }
}

// Implementation
// ---------------------

int main()
{
    // InitializeDevice();

    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);
    
    m_GLFWWindow = glfwCreateWindow(800, 600, "Test", nullptr, nullptr);

#if __APPLE__
    m_NRIWindow.metal.caMetalLayer = GetMetalLayer(m_GLFWWindow);
#endif

    InitializeDevice();


    for (uint32_t frameIndex = 0; frameIndex < UINT_MAX; frameIndex++)
    {   
        // Events
        glfwPollEvents();

        bool isActive = glfwGetWindowAttrib(m_GLFWWindow, GLFW_FOCUSED) != 0;
        if (!isActive)
        {
            frameIndex--;
            continue;
        }

        if (glfwWindowShouldClose(m_GLFWWindow))
            break;

        const uint32_t bufferedFrameIndex = frameIndex % NUM_FRAME_IN_FLIGHT;
        const Frame& frame = m_Frames[bufferedFrameIndex];

        if (frameIndex >= NUM_FRAME_IN_FLIGHT)
        {
            NRI.Wait(*m_FrameFence, 1 + frameIndex - NUM_FRAME_IN_FLIGHT);
            NRI.ResetCommandAllocator(*frame.commandAllocator);
        }

        const uint32_t currentTextureIndex = NRI.AcquireNextSwapChainTexture(*m_SwapChain);
        BackBuffer& currentBackBuffer = m_SwapChainBuffers[currentTextureIndex];

        nri::TextureBarrierDesc textureBarrierDescs = {};
        textureBarrierDescs.texture = currentBackBuffer.resource;
        textureBarrierDescs.after = {nri::AccessBits::COLOR_ATTACHMENT, nri::Layout::COLOR_ATTACHMENT};
        textureBarrierDescs.arraySize = 1;
        textureBarrierDescs.mipNum = 1;

        // Record
        nri::CommandBuffer* commandBuffer = frame.commandBuffer;
        NRI.BeginCommandBuffer(*commandBuffer, nullptr);
        {
            nri::BarrierGroupDesc barrierGroupDesc = {};
            barrierGroupDesc.textureNum = 1;
            barrierGroupDesc.textures = &textureBarrierDescs;
            NRI.CmdBarrier(*commandBuffer, barrierGroupDesc);

            nri::AttachmentsDesc attachmentsDesc = {};
            attachmentsDesc.colorNum = 1;
            attachmentsDesc.colors = &currentBackBuffer.colorAttachment;

            NRI.CmdBeginRendering(*commandBuffer, attachmentsDesc);
            {
                nri::ClearDesc clearDesc = {};
                clearDesc.attachmentContentType = nri::AttachmentContentType::COLOR;
                clearDesc.value.color32f        = { 1, 0, 0, 1 };
                NRI.CmdClearAttachments(*commandBuffer, &clearDesc, 1, nullptr, 0);


                clearDesc.value.color32f = { 0, 0, 1, 1 };

                nri::Rect rects[2];
                rects[0] = {0, 0, 400, 300};
                rects[1] = {(int16_t)400, (int16_t)300, 400, 300};

                NRI.CmdClearAttachments(*commandBuffer, &clearDesc, 1, rects, 2);
            }
            NRI.CmdEndRendering(*commandBuffer);

            textureBarrierDescs.before = textureBarrierDescs.after;
            textureBarrierDescs.after = {nri::AccessBits::UNKNOWN, nri::Layout::PRESENT};

            NRI.CmdBarrier(*commandBuffer, barrierGroupDesc);
        }
        NRI.EndCommandBuffer(*commandBuffer);

        { // Submit
            nri::QueueSubmitDesc queueSubmitDesc = {};
            queueSubmitDesc.commandBuffers = &frame.commandBuffer;
            queueSubmitDesc.commandBufferNum = 1;

            NRI.QueueSubmit(*m_GraphicsQueue, queueSubmitDesc);
        }

        // Present
        NRI.QueuePresent(*m_SwapChain);

        { // Signaling after "Present" improves D3D11 performance a bit
            nri::FenceSubmitDesc signalFence = {};
            signalFence.fence = m_FrameFence;
            signalFence.value = 1 + frameIndex;

            nri::QueueSubmitDesc queueSubmitDesc = {};
            queueSubmitDesc.signalFences = &signalFence;
            queueSubmitDesc.signalFenceNum = 1;

            NRI.QueueSubmit(*m_GraphicsQueue, queueSubmitDesc);
        }
    }

    glfwDestroyWindow(m_GLFWWindow);
    glfwTerminate();
}


/*
int main(int argc, char **argv, char **envp)
{ 
    // Create render device

    nri::AdapterDesc bestAdapterDesc = {};
    uint32_t adapterDescsNum = 1;
    nri::nriEnumerateAdapters(&bestAdapterDesc, adapterDescsNum);

    // Load Render Plugin
    // ---------------------

    // NOTE: For GetRendererPlugin() to successfully find the token, ensure the PXR_PLUGINPATH_NAME env variable is set.
    // NOTE: Technically since we are linked with the ExampleDelegate we can just directly instantiate one here but we don't for demo purpose.
    HdRendererPlugin *rendererPlugin = HdRendererPluginRegistry::GetInstance().GetRendererPlugin(TfToken("RendererPlugin"));
    TF_VERIFY(rendererPlugin != nullptr);

    // Create render delegate instance from the plugin. 
    // ---------------------

    HdRenderDelegate *renderDelegate = rendererPlugin->CreateRenderDelegate();
    TF_VERIFY(renderDelegate != nullptr);

    // Wrap our vulkan device implementation in an HDDriver
    // We could use the OpenUSD Hydra "HGI" here but for learning purposes I would rather use a from-scratch implementation.
    // ---------------------

    HdDriver customDriver{TfToken("CustomVulkanDevice"), VtValue(nullptr)};
    
    // Create render index from the delegate. 
    // ---------------------

    HdRenderIndex *renderIndex = HdRenderIndex::New(renderDelegate, { &customDriver });
    TF_VERIFY(renderIndex != nullptr);

    // Construct a scene delegate from the stock OpenUSD scene delegate implementation.
    // ---------------------

    UsdImagingDelegate *sceneDelegate = new UsdImagingDelegate(renderIndex, SdfPath::AbsoluteRootPath());
    TF_VERIFY(sceneDelegate != nullptr);

    // Load a USD Stage.
    // ---------------------

    UsdStageRefPtr usdStage = pxr::UsdStage::Open(std::string(getenv("HOME")) + "/Downloads/Kitchen_set/Kitchen_set.usd");
    TF_VERIFY(usdStage != nullptr);

    // Pipe the USD stage into the scene delegate (will create render primitives in the render delegate).
    // ---------------------

    sceneDelegate->Populate(usdStage->GetPseudoRoot());

    // Create the render tasks.
    // ---------------------

    HdxTaskController taskController(renderIndex, SdfPath("/taskController"));
    {
        auto params = HdxRenderTaskParams();
        {
            params.viewport = GfVec4i(0, 0, 800, 600);
        }

        // The "Task Controller" will automatically configure an HdxRenderTask
        // (which will create and invoke our delegate's renderpass).
        taskController.SetRenderParams(params);
        taskController.SetRenderViewport({ 0, 0, 800, 600 });
    }

    // Initialize the Hydra engine. 
    // ---------------------

    HdEngine engine;

    // Render-loop
    // ---------------------


    return 0;
}
*/