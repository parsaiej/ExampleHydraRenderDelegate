#include <ExampleDelegate/ExRenderDelegate.h>
#include <ExampleDelegate/ExRenderPass.h>
#include <ExampleDelegate/ExMesh.h>

#include <VulkanWrappers/Device.h>

#include <iostream>
#include <memory>

const TfTokenVector ExRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
};

const TfTokenVector ExRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
};

const TfTokenVector ExRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};

ExRenderDelegate::ExRenderDelegate() : HdRenderDelegate()
{
    _Initialize();
}

ExRenderDelegate::ExRenderDelegate(HdRenderSettingsMap const& settingsMap) : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void ExRenderDelegate::_Initialize()
{
    std::cout << "Creating Custom RenderDelegate" << std::endl;
    _resourceRegistry = std::make_shared<HdResourceRegistry>();
}

ExRenderDelegate::~ExRenderDelegate()
{
    _resourceRegistry.reset();
    std::cout << "Destroying Custom RenderDelegate" << std::endl;
}

// In case 
struct NoOpDeleter { void operator()(VulkanWrappers::Device* ptr) const {} };

void ExRenderDelegate::SetDrivers(HdDriverVector const& drivers)
{
    for (const auto& driver : drivers)
    {
        if (driver->name == TfToken("CustomVulkanDevice") && driver->driver.IsHolding<VulkanWrappers::Device*>())
        {
            m_GraphicsDevice = driver->driver.UncheckedGet<VulkanWrappers::Device*>();
            return;
        }
    }

    // If no driver is passed, then create it here (no window). 
    m_DefaultGraphicsDevice = std::make_unique<VulkanWrappers::Device>();

    // And set the main device resource to the default one.
    m_GraphicsDevice = m_DefaultGraphicsDevice.get();
}

TfTokenVector const& ExRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const& ExRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const& ExRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr ExRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

void ExRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
}

HdRenderPassSharedPtr ExRenderDelegate::CreateRenderPass(HdRenderIndex *index, HdRprimCollection const& collection)
{
    std::cout << "Create Custom RenderPass with Collection="  << collection.GetName() << std::endl; 
    return HdRenderPassSharedPtr(new ExRenderPass(index, collection, this));  
}

HdRprim* ExRenderDelegate::CreateRprim(TfToken const& typeId, SdfPath const& rprimId)
{
    std::cout << "Create Custom Rprim type=" << typeId.GetText() 
        << " id=" << rprimId 
        << std::endl;

    if (typeId == HdPrimTypeTokens->mesh) {
        return new ExMesh(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s", 
            typeId.GetText(), 
            rprimId.GetText());
    }
    return nullptr;
}

void ExRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    std::cout << "Destroy Custom Rprim id=" << rPrim->GetId() << std::endl;
    delete rPrim;
}

HdSprim* ExRenderDelegate::CreateSprim(TfToken const& typeId, SdfPath const& sprimId)
{
    TF_CODING_ERROR("Unknown Sprim type=%s id=%s", typeId.GetText(), sprimId.GetText());
    return nullptr;
}

HdSprim* ExRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback sprim type=%s", typeId.GetText()); 
    return nullptr;
}

void ExRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    TF_CODING_ERROR("Destroy Sprim not supported");
}

HdBprim* ExRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    TF_CODING_ERROR("Unknown Bprim type=%s id=%s", typeId.GetText(), bprimId.GetText());
    return nullptr;
}

HdBprim* ExRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_CODING_ERROR("Creating unknown fallback bprim type=%s", typeId.GetText()); 
    return nullptr;
}

void ExRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    TF_CODING_ERROR("Destroy Bprim not supported");
}

HdInstancer* ExRenderDelegate::CreateInstancer(HdSceneDelegate *delegate, SdfPath const& id)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s", id.GetText());
    return nullptr;
}

void ExRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdRenderParam* ExRenderDelegate::GetRenderParam() const
{
    return nullptr;
}