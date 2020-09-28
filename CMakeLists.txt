cmake_minimum_required(VERSION 2.8.4)

project(ffproc)

include(3rdparty.cmake)
# Set Version
set(FFPROC_MAJOR 1)
set(FFPROC_MINOR 0)
set(FFPROC_PATCH 0)
set(PROJECT_VERSION ${FFPROC_MAJOR}.${FFPROC_MINOR}.${FFPROC_PATCH})

add_subdirectory(src)
add_subdirectory(examples)
