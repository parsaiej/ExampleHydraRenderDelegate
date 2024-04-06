set(TEST_NAME StandaloneTest)
project(${TEST_NAME})

# Library
# --------------------------------------------------

add_executable(${TEST_NAME} "StandaloneTest.cpp")

# Include
# --------------------------------------------------

target_include_directories (${TEST_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories (${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/External/VulkanWrappers/Include)
target_include_directories (${TEST_NAME} PRIVATE ${PXR_INCLUDE_DIRS})

# Link
# --------------------------------------------------

target_link_libraries (${TEST_NAME} ${Vulkan_LIBRARIES})
target_link_libraries (${TEST_NAME} ${PXR_LIBRARIES})
target_link_libraries (${TEST_NAME} OpenGL::GL)
target_link_libraries (${TEST_NAME} GLEW::glew)
target_link_libraries (${TEST_NAME} VulkanWrappers)