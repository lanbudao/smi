try_compile(
    HAVE_SWRESAMPLE
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/swresample_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        swresample
    )
if(NOT HAVE_SWRESAMPLE)
    message(FATAL_ERROR "swresample library not found, it is needed.")
else()
    message(STATUS "swresample library found")
endif()
