#include "RenderPass.h"

#include <GL/glew.h>

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

    // USD-View will already have a target bound in GL, just clear it for now. 
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}