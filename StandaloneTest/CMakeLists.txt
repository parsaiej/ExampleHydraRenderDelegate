set(TEST_NAME StandaloneTest)
project(${TEST_NAME})

# Library
# --------------------------------------------------

add_executable(${TEST_NAME} "StandaloneTest.cpp")

# Include
# --------------------------------------------------

target_include_directories (${TEST_NAME} PRIVATE ${PXR_INCLUDE_DIRS})

# Link
# --------------------------------------------------

target_link_libraries (${TEST_NAME} ${PXR_LIBRARIES})