# ffproc

#### Description

Media player based on ffmpeg

#### Feature

- Cross-platform and Efficient
- Support variable speed when playing audio but does not change the pitch
- Support SDL,Qt,glfw...(only supports SDL,Qt temporarily)
- Support hardware decoding (TODO)
- Support subtitle which is picture (TODO)
- Support DVD (TODO)

#### Software Architecture


#### Build

1.New a file named "3rdparty.cmake" in the root directory

2.Set the path of the third-party library like this

/# FMPEG_DIR is necessary

set(FFMPEG_DIR /usr/local/opensource/ffmpeg)

/# If u want to add portaudio to audio output, set the PORTAUDIO_DIR

set(PORTAUDIO_DIR /usr/local/opensource/portaudio)

/# If u want to build the example of SDL, set the SDL_DIR

set(SDL_DIR  /usr/local/opensource/sdl)

3.Compile the entire project

#### Instructions


#### Reference

- QtAV,[http://https://github.com/wang-bin/QtAV](http://https://github.com/wang-bin/QtAV)
- ijkPlayer,[http://https://github.com/bilibili/ijkplayer](http://https://github.com/bilibili/ijkplayer)
- limlog,[http://https://github.com/zxhio/limlog](http://https://github.com/zxhio/limlog)
- mdk-sdk,[http://https://github.com/wang-bin/mdk-sdk](http://https://github.com/wang-bin/mdk-sdk)