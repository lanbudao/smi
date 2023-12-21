# Check sdl modules
if (EXISTS ${SDL_DIR})
    message("BUILD_EXAMPLE_SDL")
    add_subdirectory(sdl2)
endif()
