cmake_minimum_required(VERSION 2.8.12)

#project(sdlplay)
set(PROJECT_VERSION ${SMI_MAJOR}.${SMI_MINOR}.${SMI_PATCH})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CURRENT_PLATFORM})
link_directories(${SDL_DIR}/lib/${CURRENT_PLATFORM})
include_directories(
    ${SDL_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

add_executable(sdl2play sdl2play.cpp)

target_link_libraries(sdl2play smi sdl2 sdl2main)
