cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME ExampleHydraRenderDelegate)
project(${PROJECT_NAME})

# Project Config
# --------------------------------------------------

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

# Plugin install path
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Development/custom_usd_plugins")

if (APPLE)
    # In order for targets to properly load Vulkan libs etc. 
    list(APPEND CMAKE_INSTALL_RPATH "/usr/local/lib")
else()
    add_compile_definitions(NOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Set RPATH for all targets in the project
list(APPEND CMAKE_INSTALL_RPATH "@loader_path")

# User Options
# --------------------------------------------------

# This will link directly against the custom USD build that ships with the Houdini HDK. 
# This is required in order for the render delegate to be correctly loaded by houdini,
# otherwise there will be issues due to loading a plugin that was made with a different version.
option(BUILD_FOR_HOUDINI "Build for Houdini" OFF)

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

if (NOT BUILD_FOR_HOUDINI)
    # Path to a OpenUSD build / installation.
    list( APPEND CMAKE_PREFIX_PATH "~/Development/OpenUSD_Build_Debug/" )

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
    "Source/StbUsage.cpp"
    "Source/ExMesh.cpp"
    "Source/ExRendererPlugin.cpp"
    "Source/ExRenderDelegate.cpp"
    "Source/ExRenderPass.cpp"
    "Source/ExRenderBuffer.cpp"
)

# Include
# --------------------------------------------------

target_include_directories (${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/VulkanWrappers/Include)
target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/Source/Include)
target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/)

if (NOT BUILD_FOR_HOUDINI)
    # Include OpenUSD headers if we aren't building with Houdini HDK framework.
    target_include_directories (${PROJECT_NAME} PRIVATE ${PXR_INCLUDE_DIRS})
endif()

# Link
# --------------------------------------------------

target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${PROJECT_NAME} OpenGL::GL)
target_link_libraries (${PROJECT_NAME} GLEW::glew)
target_link_libraries (${PROJECT_NAME} VulkanWrappers)

if (NOT BUILD_FOR_HOUDINI)
    # Link against OpenUSD directly. 
    target_link_libraries (${PROJECT_NAME} ${PXR_LIBRARIES})
else()
    # Warning: On macOS some recent versions of Houdini 20.x will not correctly link due to a Houdini.framework 
    # that is missing the necessary symbolic links. 

    # Link against HDK (contains SideFX custom USD build) + Library.
    target_link_libraries (${PROJECT_NAME} ${Python_LIBRARIES})
    target_link_libraries (${PROJECT_NAME} Houdini)
endif()

# Install Config
# --------------------------------------------------

# Plugin install path
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Development/custom_usd_plugins")

# Move the library + plugin info to a location that USD checks plugins for. 
# Note: This location should be pointed to by the $PXR_PLUGINPATH_NAME env variable. 
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION .)
install(TARGETS VulkanWrappers  LIBRARY DESTINATION .)
if (APPLE)
    install(TARGETS MetalUtility    LIBRARY DESTINATION .)
endif()
install(FILES ${CMAKE_SOURCE_DIR}/Source/plugInfo.json DESTINATION ExampleHydraRenderDelegate/resources/)

# Standalone Executable Test
# --------------------------------------------------

if (NOT BUILD_FOR_HOUDINI)
    add_subdirectory(StandaloneTest/)
endif()