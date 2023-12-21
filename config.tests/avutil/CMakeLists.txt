include(../../3rdparty.cmake)

try_compile(
    HAVE_AVUTIL
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avutil_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avutil
    )
if(NOT HAVE_AVUTIL)
    message(FATAL_ERROR "avutil library not found, it is needed.")
else()
    message(STATUS "avutil library found")
endif()
