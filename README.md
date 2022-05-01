# SMI

#### Description

Simple media interface based on ffmpeg(refer to QtAV)

#### Feature

- Cross-platform and Efficient
- Support variable speed when playing audio but does not change the pitch(use soundtouch)
- Support switching media track dynamic
- Support SDL,Qt,glfw,glut...(only supports SDL,Qt temporarily)
- Support extra subtitle(support .srt,.ass,.ssa) and internal subtitle(support .srt,.ass,.ssa,.sub)
- Support hardware decoding (TODO)
- Support DVD (TODO)

#### Software Architecture


#### Build

1.New a file named "3rdparty.cmake" in the root directory

2.Set the path of the third-party library like this

\#FFMPEG_DIR is necessary

set(FFMPEG_DIR C:/msys64/usr/local/ffmpeg)

\# Optional below

\# If u want to add portaudio to audio output, set the PORTAUDIO_DIR

set(PORTAUDIO_DIR E:/source/opensource/portaudio)

\# if u want to add subtitle support, set the FREETYPE_DIR

set(FREETYPE_DIR "C:\\msys64\\usr\\local\\freetype")

\# If u want to build the example of SDL, set the SDL_DIR
set(SDL_DIR E:/source/opensource/sdl)

3.Compile the entire project

#### Instructions


#### Reference

- QtAV,[https://github.com/wang-bin/QtAV](https://github.com/wang-bin/QtAV)
- ijkPlayer,[https://github.com/bilibili/ijkplayer](https://github.com/bilibili/ijkplayer)
- limlog,[https://github.com/zxhio/limlog](https://github.com/zxhio/limlog)
- mdk-sdk,[https://github.com/wang-bin/mdk-sdk](https://github.com/wang-bin/mdk-sdk)