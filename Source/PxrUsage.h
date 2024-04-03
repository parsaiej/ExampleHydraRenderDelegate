#ifdef __APPLE__
    // MacOS fix for bug inside USD. 
    #define unary_function __unary_function
#endif

#include <pxr/pxr.h>
#include <pxr/base/tf/staticTokens.h>

// Imaging (Hydra)
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/mesh.h"