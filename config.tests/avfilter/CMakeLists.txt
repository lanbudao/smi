include(../../3rdparty.cmake)

try_compile(
    HAVE_AVFILTER
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avfilter_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avfilter
    )
if(NOT HAVE_AVFILTER)
    message(FATAL_ERROR "avfilter library not found, it is needed.")
else()
    message(STATUS "avfilter library found")
endif()
