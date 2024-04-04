cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME ExampleHydraRenderDelegate)
project(${PROJECT_NAME})

# Project Config
# --------------------------------------------------

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

# User Options
# --------------------------------------------------

# This will link directly against the custom USD build that ships with the Houdini HDK. 
# This is required in order for the render delegate to be correctly loaded by houdini,
# otherwise there will be issues due to loading a plugin that was made with a different version.
option(BUILD_FOR_HOUDINI "Build for Houdini" ON)

# Configure misc. packages
# --------------------------------------------------

# Enable a define in the wrappers to enable Vulkan validation layers. 
add_definitions(-DENABLE_VALIDATION_LAYERS)

add_subdirectory(External/VulkanWrappers)

# Find Packages
# --------------------------------------------------

find_package(Vulkan  REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(GLEW    REQUIRED)

if (!BUILD_FOR_HOUDINI)
    # Path to a OpenUSD build / installation.
    list( APPEND CMAKE_PREFIX_PATH "~/Development/OpenUSD_Build/" )

    # Configure a OpenUSD installation. 
    find_package(pxr REQUIRED)
else()
    # Path to a Houdini HDK installation.
    list( APPEND CMAKE_PREFIX_PATH "/Applications/Houdini/Houdini20.0.506/Frameworks/Houdini.framework/Versions/20.0/Resources/toolkit/cmake/" )

    # Note: We also will need Python in order to link with Houdini HDK correctly. 
    # Note: For Houdini 20.x you will need Python 3.10 (i.e. brew install python@3.10). 
    find_package(Python REQUIRED COMPONENTS Development)

    # Info: https://www.sidefx.com/docs/hdk/_h_d_k__intro__compiling.html
    find_package(Houdini REQUIRED)
endif()

# Library
# --------------------------------------------------

add_library(${PROJECT_NAME} SHARED
    "Source/PxrUsage.cpp"
    "Source/Mesh.cpp"
    "Source/RendererPlugin.cpp"
    "Source/RenderDelegate.cpp"
    "Source/RenderPass.cpp"
)

# Include
# --------------------------------------------------

target_include_directories (${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/VulkanWrappers/Include)

if (!BUILD_FOR_HOUDINI)
    # Include OpenUSD headers if we aren't building with Houdini HDK framework.
    target_include_directories (${PROJECT_NAME} PRIVATE ${PXR_INCLUDE_DIRS})
endif()

# Link
# --------------------------------------------------

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} OpenGL::GL)
target_link_libraries (${PROJECT_NAME} GLEW::glew)
target_link_libraries (${PROJECT_NAME} VulkanWrappers)

if (!BUILD_FOR_HOUDINI)
    # Link against OpenUSD directly. 
    target_link_libraries (${PROJECT_NAME} ${PXR_LIBRARIES})
else()
    # Link against HDK (contains SideFX custom USD build) + Library.
    target_link_libraries (${PROJECT_NAME} ${Python_LIBRARIES})
    target_link_libraries (${PROJECT_NAME} Houdini)

    # Warning: Some recent versions of Houdini 20.x will not correctly link due to a Houdini.framework 
    # that is missing the necessary symbolic links. 
endif()

# Install
# --------------------------------------------------

