cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME ExampleHydraRenderDelegate)
project(${PROJECT_NAME})

# Project Config
# --------------------------------------------------

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

# Configure misc. packages
# --------------------------------------------------

add_definitions(-DENABLE_VALIDATION_LAYERS)
add_subdirectory(External/VulkanWrappers)

# Find Packages
# --------------------------------------------------

find_package(Vulkan  REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(GLEW    REQUIRED)
# find_package(pxr     REQUIRED)

# Path to a Houdini HDK installation.
list( APPEND CMAKE_PREFIX_PATH "/Applications/Houdini/Houdini20.0.506/Frameworks/Houdini.framework/Versions/20.0/Resources/toolkit/cmake/" )

# Info: https://www.sidefx.com/docs/hdk/_h_d_k__intro__compiling.html
find_package(Python  REQUIRED COMPONENTS Development)
find_package(Houdini REQUIRED)

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
# target_include_directories (${PROJECT_NAME} PRIVATE ${PXR_INCLUDE_DIRS})

# Link
# --------------------------------------------------

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} OpenGL::GL)
target_link_libraries (${PROJECT_NAME} GLEW::glew)
target_link_libraries (${PROJECT_NAME} VulkanWrappers)
# target_link_libraries (${PROJECT_NAME} ${PXR_LIBRARIES})

target_link_libraries (${PROJECT_NAME} ${Python_LIBRARIES})
target_link_libraries (${PROJECT_NAME} Houdini)