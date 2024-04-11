#include <VulkanWrappers/Device.h>
#include <VulkanWrappers/Window.h>
#include <VulkanWrappers/Image.h>
#include <VulkanWrappers/Shader.h>
#include <VulkanWrappers/Buffer.h>

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

PXR_NAMESPACE_USING_DIRECTIVE

using namespace VulkanWrappers;

// Implementation
// ---------------------

int main(int argc, char **argv, char **envp)
{ 
    // Create window. 
    Window window("Standalone Vulkan Hydra Executable", 800, 600);

    // Initialize graphics device usage with the window. 
    Device device(&window);

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

    HdDriver customDriver{TfToken("CustomVulkanDevice"), VtValue(&device)};
    
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

    // Handle to current frame to write commands to. 
    Frame frame;

    while (window.NextFrame(&device, &frame))
    {
        // Forward the current backbuffer and commandbuffer to the delegate. 
        // This feels quite hacky, open to suggestions. 
        // There might be a simpler way to manage this by writing my own HdTask, but
        // it would require sacrificing the simplicity that HdxTaskController offers.
        renderDelegate->SetRenderSetting(TfToken("CurrentFrame"), VtValue(&frame));

        // Invoke Hydra!
        auto renderTasks = taskController.GetRenderingTasks();
        engine.Execute(renderIndex, &renderTasks);

        window.SubmitFrame(&device, &frame);
    }

    return 0;
}