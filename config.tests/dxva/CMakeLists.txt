try_compile(
    HAVE_DXVA
    ${CMAKE_CURRENT_BINARY_DIR}/config.tests.temp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/dxva_test.cpp
    CMAKE_FLAGS
        "-DINCLUDE_DIRECTORIES=${FFMPEG_DIR}/include"
        "-DLINK_DIRECTORIES=${FFMPEG_DIR}/lib"
    COMPILE_DEFINITIONS
        -D__STDC_CONSTANT_MACROS
    LINK_LIBRARIES
        avcodec
    )
if(NOT HAVE_DXVA)
    message(STATUS "dxva not found.")
else()
    message(STATUS "dxva found")
endif()
