cmake_minimum_required(VERSION 2.8.4)

project(sdlplay)
set(PROJECT_VERSION ${FFPROC_MAJOR}.${FFPROC_MINOR}.${FFPROC_PATCH})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
link_directories(${SDL_DIR}/lib ${CMAKE_SOURCE_DIR}/bin/${CMAKE_INTDIR})
include_directories(
    ${SDL_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

add_executable(sdl2play sdl2play.cpp)

target_link_libraries(sdl2play smi sdl2 sdl2main)
