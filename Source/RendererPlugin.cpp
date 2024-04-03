#include "RendererPlugin.h"
#include "RenderDelegate.h"

TF_REGISTRY_FUNCTION(TfType)
{
    // Register the plugin with the renderer plugin system.
    HdRendererPluginRegistry::Define<RendererPlugin>();
}

HdRenderDelegate* RendererPlugin::CreateRenderDelegate()
{
    return new RenderDelegate();
}

HdRenderDelegate* RendererPlugin::CreateRenderDelegate(HdRenderSettingsMap const& settingsMap)
{
    return new RenderDelegate(settingsMap);
}

void RendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}

bool RendererPlugin::IsSupported(bool /* gpuEnabled */) const
{
    // Nothing more to check for now, we assume if the plugin loads correctly
    // it is supported.
    return true;
}