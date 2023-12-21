include(../../3rdparty.cmake)

try_compile(
    HAVE_AVFORMAT
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avformat_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avformat
    )
if(NOT HAVE_AVFORMAT)
    message(FATAL_ERROR "avformat library not found, it is needed.")
else()
    message(STATUS "avformat library found")
endif()
