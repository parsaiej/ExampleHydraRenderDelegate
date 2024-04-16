if (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(QUARTZ_CORE QuartzCore REQUIRED)

    add_library(MetalUtility SHARED "MetalUtility.m")

    # Link
    target_link_libraries(MetalUtility glfw)
    target_link_libraries(MetalUtility ${QUARTZ_CORE})
    target_link_libraries(MetalUtility ${COCOA_LIBRARY})
endif ()