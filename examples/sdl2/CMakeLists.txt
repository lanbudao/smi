#project(sdlplay)
set(PROJECT_VERSION ${SMI_MAJOR}.${SMI_MINOR}.${SMI_PATCH})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CURRENT_COMPILER}/${CURRENT_PLATFORM})
link_directories(${SDL_DIR}/lib/${CURRENT_PLATFORM})
include_directories(
    ${SDL_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

add_executable(sdl2play sdl2play.cpp)
list(APPEND EXTRA_LIB smi sdl2)
if(MSVC)
    list(APPEND EXTRA_LIB sdl2main)
endif()
target_link_libraries(sdl2play ${EXTRA_LIB})
if(NOT MSVC)
    target_compile_definitions(sdl2play PRIVATE -DSDL_MAIN_HANDLED)
endif()
