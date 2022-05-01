
# FFMPEG_DIR is necessary
# Default, I will find libraries from directory named 'lib' you set, not 'bin'.
# For example, I will find 'avformat.lib' form 'C:/msys64/usr/local/ffmpeg/lib'.

set(FFMPEG_DIR C:/msys64/usr/local/ffmpeg)

# Optional below

# if u want to add subtitle support, set the LIBASS_DIR
set(LIBASS_DIR "C:\\msys64\\usr\\local\\libass")

# If u want to add portaudio to audio output, set the PORTAUDIO_DIR
set(PORTAUDIO_DIR "C:\\msys64\\usr\\local\\portaudio")

# If u want to support DVD, set the LIBDVDREAD_DIR and LIBDVDNAV_DIR
set(LIBDVDREAD_DIR "C:\\msys64\\usr\\local\\libdvdread")
set(LIBDVDNAV_DIR "C:\\msys64\\usr\\local\\libdvdnav")

# If u want to build the example of SDL, set the SDL_DIR
set(SDL_DIR H:\\source_laptop\\opensource\\SDL)