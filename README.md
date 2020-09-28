# ffproc

#### Description

Media player based on ffmpeg

#### Feature

1. Cross-platform and Efficient
2. Support SDL,Qt,glfw...(only supports SDL temporarily)
2. Support hardware decoding (TODO)
3. Support subtitle which is picture (TODO)
4. Support DVD (TODO)

#### Software Architecture


#### Build

1.  New a file named "3rdparty.cmake" in the root directory
2.  Set the path of the third-party library like this
#FFMPEG_DIR is necessary
set(FFMPEG_DIR /usr/local/opensource/ffmpeg)
#If u want to add portaudio to audio output, set the PORTAUDIO_DIR
set(PORTAUDIO_DIR /usr/local/opensource/portaudio)
#If u want to build the example of SDL, set the SDL_DIR
set(SDL_DIR  /usr/local/opensource/sdl)
3.  Compile the entire project

#### Instructions

