# FFMPEG_DIR is necessary
# Default, I will find libraries from directory named 'lib' you set, not 'bin'.
# For example, I will find 'avformat.lib' form 'C:/msys64/usr/local/ffmpeg/lib'.

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CURRENT_PLATFORM "x64")
    message(STATUS "Current Platform is ${CURRENT_PLATFORM}")
else()
    set(CURRENT_PLATFORM "x86")
    message(STATUS "Current Platform is ${CURRENT_PLATFORM}")
endif()
if(MSVC)
    set(CURRENT_COMPILER "msvc")
elseif(MINGW)
    set(CURRENT_COMPILER "mingw")
elseif(GNU)
    set(CURRENT_COMPILER "gnu")
endif()

set(FFMPEG_DIR "H:\\source_laptop\\opensource\\ffmpeg_")
string(APPEND FFMPEG_DIR ${CURRENT_PLATFORM})
# Optional below
# if u want to add subtitle support, set the LIBASS_DIR
#set(LIBASS_DIR "C:\\msys64\\usr\\local\\libass")

# If u want to add portaudio to audio output, set the PORTAUDIO_DIR
#set(PORTAUDIO_DIR "C:\\msys64\\usr\\local\\portaudio")

# If u want to support DVD, set the LIBDVDREAD_DIR and LIBDVDNAV_DIR
#set(LIBDVDREAD_DIR "C:\\msys64\\usr\\local\\libdvdread")
#set(LIBDVDNAV_DIR "C:\\msys64\\usr\\local\\libdvdnav")

# If u want to build the example of SDL, set the SDL_DIR
if(MSVC)
    set(SDL_DIR "H:\\source_laptop\\opensource\\SDL2")
elseif(MINGW)
    set(SDL_DIR "H:\\source_laptop\\opensource\\SDL2_mingw")
endif()
