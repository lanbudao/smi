try_compile(
    HAVE_D3D11VA
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/d3d11va_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avcodec
    )
if(NOT HAVE_D3D11VA)
    message(STATUS "d3d11va not found.")
else()
    message(STATUS "d3d11va found")
endif()
