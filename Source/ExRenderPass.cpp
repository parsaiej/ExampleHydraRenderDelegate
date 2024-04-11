#include <ExampleDelegate/ExRenderPass.h>
#include <ExampleDelegate/ExRenderDelegate.h>
#include <ExampleDelegate/StbUsage.h>

#include <pxr/usd/ar/defaultResolver.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolvedPath.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/imaging/hd/camera.h>

#include <iostream>

ExRenderPass::ExRenderPass(HdRenderIndex *index, HdRprimCollection const &collection, ExRenderDelegate* renderDelegate) 
    : HdRenderPass(index, collection), m_Owner(renderDelegate)
{
}

ExRenderPass::~ExRenderPass() 
{
}

void ExRenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
}