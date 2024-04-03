#include "RenderPass.h"

#include <iostream>

RenderPass::RenderPass(HdRenderIndex *index, HdRprimCollection const &collection) : HdRenderPass(index, collection)
{
}

RenderPass::~RenderPass()
{
    std::cout << "Destroying renderPass" << std::endl;
}

void RenderPass::_Execute(HdRenderPassStateSharedPtr const& renderPassState, TfTokenVector const &renderTags)
{
    std::cout << "=> Execute RenderPass" << std::endl;
}