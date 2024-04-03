cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME ExampleHydraRenderDelegate)
project(${PROJECT_NAME})

# Project Config
# --------------------------------------------------

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DENABLE_VALIDATION_LAYERS)

# Path to a USD installation.
set (CMAKE_PREFIX_PATH "~/Development/OpenUSD_Build/")

# Configure misc. packages
# --------------------------------------------------

add_subdirectory(External/VulkanWrappers)

# Find Packages
# --------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(pxr    REQUIRED)

# Library
# --------------------------------------------------

add_library(${PROJECT_NAME} SHARED
    "Source/PxrUsage.cpp"
    "Source/Mesh.cpp"
    "Source/RendererPlugin.cpp"
    "Source/RenderDelegate.cpp"
    "Source/RenderPass.cpp")

# Include
# --------------------------------------------------

target_include_directories (${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/VulkanWrappers/Include)
target_include_directories (${PROJECT_NAME} PRIVATE ${PXR_INCLUDE_DIRS})

# Link
# --------------------------------------------------

target_link_libraries (${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} PRIVATE OpenGL::GL)
target_link_libraries (${PROJECT_NAME} PRIVATE GLEW::glew)
target_link_libraries (${PROJECT_NAME} PRIVATE VulkanWrappers)
target_link_libraries (${PROJECT_NAME} PRIVATE ${PXR_LIBRARIES})