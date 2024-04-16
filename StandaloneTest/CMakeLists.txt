set(TEST_NAME StandaloneTest)
project(${TEST_NAME})

# Library
# --------------------------------------------------

add_executable(${TEST_NAME} "StandaloneTest.cpp")

# Include
# --------------------------------------------------

target_include_directories (${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/)
target_include_directories (${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/glfw/include)
target_include_directories (${TEST_NAME} PRIVATE ${PXR_INCLUDE_DIRS})

# Link
# --------------------------------------------------

if (APPLE)
    # We need to link with Vulkan in order for NRI to use the Vulkan loader.
    target_link_libraries (${TEST_NAME} ${Vulkan_LIBRARIES})
endif()

target_link_libraries (${TEST_NAME} ${PXR_LIBRARIES})
target_link_libraries (${TEST_NAME} NRI)

if (APPLE)
    # Metal Utility is linked with glfw already.
    target_link_libraries (${TEST_NAME} MetalUtility)
else()
    target_link_libraries (${TEST_NAME} glfw)
endif()