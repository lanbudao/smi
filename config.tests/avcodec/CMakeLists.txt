include(../../3rdparty.cmake)

try_compile(
    HAVE_AVCODEC
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avcodec_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avcodec
    )
if(NOT HAVE_AVCODEC)
    message(FATAL_ERROR "avcodec library not found, it is needed.")
else()
    message(STATUS "avcodec library found")
endif()
