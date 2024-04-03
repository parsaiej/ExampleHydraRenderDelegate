#ifndef MESH
#define MESH

#include "PxrUsage.h"

PXR_NAMESPACE_USING_DIRECTIVE

/// \class Mesh
///
/// This class is an example of a Hydra Rprim, or renderable object, and it
/// gets created on a call to HdRenderIndex::InsertRprim() with a type of
/// HdPrimTypeTokens->mesh.
///
/// The prim object's main function is to bridge the scene description and the
/// renderable representation. The Hydra image generation algorithm will call
/// HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
/// Sync() for each mesh with new data.
///
/// Sync() is passed a set of dirtyBits, indicating which scene buffers are
/// dirty. It uses these to pull all of the new scene data and constructs
/// updated geometry objects.
///
/// An rprim's state is lazily populated in Sync(); matching this, Finalize()
/// can do the heavy work of releasing state (such as handles into the top-level
/// scene), so that object population and existence aren't tied to each other.
///
class Mesh final : public HdMesh 
{
public:
    HF_MALLOC_TAG_NEW("new Mesh");

    Mesh(SdfPath const& id);
    ~Mesh() override = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and points data to build
    /// the geometry object in the scene graph.
    ///   \return The initial dirty state this mesh wants to query.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///
    /// This function is told which scene data to pull through the
    /// dirtyBits parameter. The first time it's called, dirtyBits comes
    /// from _GetInitialDirtyBits(), which provides initial dirty state,
    /// but after that it's driven by invalidation tracking in the scene
    /// delegate.
    ///
    /// The contract for this function is that the prim can only pull on scene
    /// delegate buffers that are marked dirty. Scene delegates can and do
    /// implement just-in-time data schemes that mean that pulling on clean
    /// data will be at best incorrect, and at worst a crash.
    ///
    /// This function is called in parallel from worker threads, so it needs
    /// to be threadsafe; calls into HdSceneDelegate are ok.
    ///
    /// Reprs are used by hydra for controlling per-item draw settings like
    /// flat/smooth shaded, wireframe, refined, etc.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam State.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///   \param reprToken A specifier for which representation to draw with.
    ///
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam*   renderParam,
        HdDirtyBits*     dirtyBits,
        TfToken const    &reprToken) override;

protected:
    // Initialize the given representation of this Rprim.
    // This is called prior to syncing the prim, the first time the repr
    // is used.
    //
    // reprToken is the name of the repr to initalize.
    //
    // dirtyBits is an in/out value.  It is initialized to the dirty bits
    // from the change tracker.  InitRepr can then set additional dirty bits
    // if additional data is required from the scene delegate when this
    // repr is synced.  InitRepr occurs before dirty bit propagation.
    //
    // See HdRprim::InitRepr()
    void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    // This callback from Rprim gives the prim an opportunity to set
    // additional dirty bits based on those already set.  This is done
    // before the dirty bits are passed to the scene delegate, so can be
    // used to communicate that extra information is needed by the prim to
    // process the changes.
    //
    // The return value is the new set of dirty bits, which replaces the bits
    // passed in.
    //
    // See HdRprim::PropagateRprimDirtyBits()
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;
};

#endif