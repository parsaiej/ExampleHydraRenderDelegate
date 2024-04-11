#ifndef RENDERER_DELEGATE
#define RENDERER_DELEGATE

#include "PxrUsage.h"

PXR_NAMESPACE_USING_DIRECTIVE

class ExRenderDelegate final : public HdRenderDelegate
{
public:

    // Render Delegate Implementation
    // ----------------------------------

    /// Render delegate constructor. 
    ExRenderDelegate();

    /// Render delegate constructor. 
    ExRenderDelegate(HdRenderSettingsMap const& settingsMap);

    /// Render delegate destructor.
    virtual ~ExRenderDelegate();

    void SetDrivers(HdDriverVector const& drivers) override;

    /// Supported types
    const TfTokenVector &GetSupportedRprimTypes() const override;
    const TfTokenVector &GetSupportedSprimTypes() const override;
    const TfTokenVector &GetSupportedBprimTypes() const override;

    // Basic value to return from the RD
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    // Prims
    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection) override;

    HdInstancer *CreateInstancer(HdSceneDelegate *delegate, SdfPath const& id) override;
    void DestroyInstancer(HdInstancer *instancer) override;

    HdRprim *CreateRprim(TfToken const& typeId, SdfPath const& rprimId) override;
    void DestroyRprim(HdRprim *rPrim) override;

    HdSprim *CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    void DestroySprim(HdSprim *sprim) override;

    HdBprim *CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    void DestroyBprim(HdBprim *bprim) override;

    void CommitResources(HdChangeTracker *tracker) override;

    HdRenderParam *GetRenderParam() const override;

private:

    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    void _Initialize();

    HdResourceRegistrySharedPtr _resourceRegistry;
};

#endif