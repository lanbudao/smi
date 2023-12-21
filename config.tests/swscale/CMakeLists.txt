include(../../3rdparty.cmake)

try_compile(
    HAVE_SWSCALE
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/swscale_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        swscale
    )
if(NOT HAVE_SWSCALE)
    message(FATAL_ERROR "swscale library not found, it is needed.")
else()
    message(STATUS "swscale library found")
endif()
