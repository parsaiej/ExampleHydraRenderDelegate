#include "RenderDelegate.h"
#include "Mesh.h"

#include <iostream>

const TfTokenVector RenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector RenderDelegate::SUPPORTED_SPRIM_TYPES =
{
};

const TfTokenVector RenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

RenderDelegate::RenderDelegate() : HdRenderDelegate()
{
    _Initialize();
}

RenderDelegate::RenderDelegate(HdRenderSettingsMap const& settingsMap) : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void RenderDelegate::_Initialize()
{
    std::cout << "Creating Custom RenderDelegate" << std::endl;
    _resourceRegistry = std::make_shared<HdResourceRegistry>();
}

RenderDelegate::~RenderDelegate()
{
    _resourceRegistry.reset();
    std::cout << "Destroying Custom RenderDelegate" << std::endl;
}

TfTokenVector const& RenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const& RenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const& RenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr RenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

void RenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    std::cout << "=> CommitResources RenderDelegate" << std::endl;
}

HdRenderPassSharedPtr RenderDelegate::CreateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection)
{
    std::cout << "Create Custom RenderPass with Collection="  << collection.GetName() << std::endl; 
    return nullptr; //  HdRenderPassSharedPtr(new HdTinyRenderPass(index, collection));  
}

HdRprim* RenderDelegate::CreateRprim(TfToken const& typeId, SdfPath const& rprimId)
{
    std::cout << "Create Custom Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << std::endl;

    if (typeId == HdPrimTypeTokens->mesh) {
        return new Mesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
            typeId.GetText(), 
            rprimId.GetText());
    }
    return nullptr;
}

void RenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    std::cout << "Destroy Custom Rprim id=" << rPrim->GetId() << std::endl;
    delete rPrim;
}

HdSprim* RenderDelegate::CreateSprim(TfToken const& typeId, SdfPath const& sprimId)
{
    TF_CODING_ERROR("Unknown Sprim type=%s id=%s", typeId.GetText(), sprimId.GetText());
    return nullptr;
}

HdSprim* RenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback sprim type=%s", typeId.GetText()); 
    return nullptr;
}

void RenderDelegate::DestroySprim(HdSprim *sPrim)
{
    TF_CODING_ERROR("Destroy Sprim not supported");
}

HdBprim* RenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    TF_CODING_ERROR("Unknown Bprim type=%s id=%s", typeId.GetText(), bprimId.GetText());
    return nullptr;
}

HdBprim* RenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback bprim type=%s", typeId.GetText()); 
    return nullptr;
}

void RenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    TF_CODING_ERROR("Destroy Bprim not supported");
}

HdInstancer* RenderDelegate::CreateInstancer(HdSceneDelegate *delegate, SdfPath const& id)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s", id.GetText());
    return nullptr;
}

void RenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdRenderParam* RenderDelegate::GetRenderParam() const
{
    return nullptr;
}