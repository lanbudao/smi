cmake_minimum_required(VERSION 2.8.12)

# Check sdl modules
if (EXISTS ${SDL_DIR})
    message("BUILD_EXAMPLE_SDL")
    add_subdirectory(sdl2)
endif()
