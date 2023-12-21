include(../../3rdparty.cmake)

try_compile(
    HAVE_AVDEVICE
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avdevice_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avdevice
    )
if(NOT HAVE_AVDEVICE)
    message(FATAL_ERROR "avdevice library not found, it is needed.")
else()
    message(STATUS "avdevice library found")
endif()
