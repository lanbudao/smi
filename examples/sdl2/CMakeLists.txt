cmake_minimum_required(VERSION 2.8.4)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

link_directories(${FFMPEG_DIR}/bin ${SDL_DIR}/lib)
include_directories(${FFMPEG_DIR}/include
    ${SDL_DIR}/include
    ${PROJECT_SOURCE_DIR}/src)

add_executable(sdl2play sdl2play.cpp)

target_link_libraries(sdl2play ffproc sdl2 sdl2main)
