#include "Mesh.h"

#include <iostream>

Mesh::Mesh(SdfPath const& id)
    : HdMesh(id)
{
}

HdDirtyBits Mesh::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::Clean | HdChangeTracker::DirtyTransform;
}

HdDirtyBits Mesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void Mesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{

}

void Mesh::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits,
                TfToken const   &reprToken)
{
    std::cout << "* (multithreaded) Sync Tiny Mesh id=" << GetId() << std::endl;
}
