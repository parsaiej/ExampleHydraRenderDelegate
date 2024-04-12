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

// NRI

// NRI: core & common extensions
#include <NRI/Include/NRI.h>
#include <NRI/Include/Extensions/NRIDeviceCreation.h>
#include <NRI/Include/Extensions/NRIHelper.h>
#include <NRI/Include/Extensions/NRIStreamer.h>
#include <NRI/Include/Extensions/NRISwapChain.h>

PXR_NAMESPACE_USING_DIRECTIVE

// Implementation
// ---------------------

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