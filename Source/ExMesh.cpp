#include <ExampleDelegate/ExMesh.h>

#include <iostream>

ExMesh::ExMesh(SdfPath const& id)
    : HdMesh(id)
{
}

HdDirtyBits ExMesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean | HdChangeTracker::DirtyTransform;
}

HdDirtyBits ExMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void ExMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}

void ExMesh::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits,
                TfToken const   &reprToken)
{
    std::cout << "* (multithreaded) Sync Tiny Mesh id=" << GetId() << std::endl;
}
