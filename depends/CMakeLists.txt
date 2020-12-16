cmake_minimum_required(VERSION 2.8.4)

# soundtouch
add_subdirectory(soundtouch)

# ftgl
# check freetype modules
if (EXISTS ${FREETYPE_DIR})
    message("BUILD_FTGL_STATIC_LIB")
    add_subdirectory(ftgl)
endif()