#ifndef RENDERER_PLUGIN
#define RENDERER_PLUGIN

#include "PxrUsage.h"

PXR_NAMESPACE_USING_DIRECTIVE

class RendererPlugin final : public HdRendererPlugin
{
public:
    RendererPlugin() = default;
    virtual ~RendererPlugin() = default;

    // Renderer Plugin Interface Implementation
    // ----------------------------------------------

    /// Construct a new render delegate of type HdTinyRenderDelegate.
    virtual HdRenderDelegate *CreateRenderDelegate() override;

    /// Construct a new render delegate of type HdTinyRenderDelegate.
    virtual HdRenderDelegate *CreateRenderDelegate(HdRenderSettingsMap const& settingsMap) override;

    /// Destroy a render delegate created by this class's CreateRenderDelegate.
    virtual void DeleteRenderDelegate(HdRenderDelegate *renderDelegate) override;

    /// Checks to see if the plugin is supported on the running system.
    virtual bool IsSupported(bool gpuEnabled = true) const override;
};

#endif