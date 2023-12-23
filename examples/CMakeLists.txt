# Check sdl modules
if (EXISTS ${SDL_DIR})
    message(STATUS "build example sdl2player")
    add_subdirectory(sdl2)
endif()
