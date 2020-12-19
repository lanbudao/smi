cmake_minimum_required(VERSION 2.8.4)

set(MODULE smi)

# Use C++11
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    message(STATUS "optional:-std=c++11")
#endif(CMAKE_COMPILER_IS_GNUCXX)

list(APPEND INCLUDE_DIR .
        sdk
        glad/include
        decoder
        output
        private
        renderer
        filter
        util)

# Check ffmpeg modules
if (EXISTS ${FFMPEG_DIR})
    list(APPEND EXTRA_INCLUDE ${FFMPEG_DIR}/include)
    set(AVMODULES avformat avcodec swscale avutil)
    foreach(av ${AVMODULES})
        string(TOUPPER ${av} AV)
        find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH NO_SYSTEM_ENVIRONMENT_PATH)
        message("HAVE_${AV}: ${HAVE_${AV}}")
        if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
            set(HAVE_${AV} 1)
#            list(APPEND EXTRA_LIBS ${av})
            find_library(${av}LIB ${av} ${FFMPEG_DIR}/lib)
            list(APPEND EXTRA_LIBS ${${av}LIB})
            list(APPEND EXTRA_DEFS "-DSMI_HAVE_${AV}=1")
        else()
            message(FATAL_ERROR "${av} is required")
        endif()
    endforeach()

    foreach(av avfilter avdevice avresample swresample)
        string(TOUPPER ${av} AV)
        find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH)
        message("HAVE_${AV}: ${HAVE_${AV}}")
        if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
            set(HAVE_${AV} 1)
#            list(APPEND EXTRA_LIBS ${av})
            find_library(${av}LIB ${av} ${FFMPEG_DIR}/lib)
            list(APPEND EXTRA_LIBS ${${av}LIB})
            list(APPEND EXTRA_DEFS "-DSMI_HAVE_${AV}=1")
            list(APPEND AVMODULES ${av})
        endif()
    endforeach()

    if (NOT HAVE_SWRESAMPLE)
        if (NOT HAVE_AVRESAMPLE)
            message(FATAL_ERROR "swresample or avresample is required")
        endif()
    endif()
endif()

# Check PortAudio modules
if (EXISTS ${PORTAUDIO_DIR})
    message("HAVE-PORTAUDIO: " ${PORTAUDIO_DIR})
    list(APPEND EXTRA_INCLUDE ${PORTAUDIO_DIR}/include)
#    link_directories(${PORTAUDIO_DIR}/lib)
#    list(APPEND EXTRA_LIBS portaudio)
    find_library(LIB_PORTAUDIO portaudio ${PORTAUDIO_DIR}/lib)
    list(APPEND EXTRA_LIBS ${LIB_PORTAUDIO})
    list(APPEND EXTRA_DEFS -DSMI_HAVE_PORTAUDIO=1)
    list(APPEND SOURCES output/audio/AudioOutputPortAudio.cpp)
endif()

IF (WIN32)
    list(APPEND EXTRA_DEFS -DSMI_HAVE_DSOUND=1)
    list(APPEND SOURCES output/audio/AudioOutputdsound.cpp)
ELSEIF (APPLE)
    MESSAGE(STATUS "Now is Apple systens.")
ELSEIF (UNIX)
    MESSAGE(STATUS "Now is UNIX-like OS's.")
ENDIF()

list(APPEND EXTRA_DEFS -DBUILD_SMI_LIB -D__STDC_CONSTANT_MACROS)

#file(GLOB SDK_HEADERS *.h)
#file(GLOB SDK_HEADERS_SDK sdk/*.h)
#file(GLOB SDK_HEADERS_RENDERER renderer/*.h)
#file(GLOB SDK_HEADERS_UTIL util/*.h)
#file(GLOB SDK_PRIVATE_HEADERS private/*.h)

# Load all source file without heareders
#aux_source_directory(. SOURCES)
#aux_source_directory(util SOURCES)
#aux_source_directory(filter SOURCES)
#aux_source_directory(decoder SOURCES)

list(APPEND HEADERS
	AVClock.h
    AVDecoder.h
    AVDemuxThread.h
    AVOutput.h
    AVThread.h
    AudioDecoder.h
    AudioFormat.h
    AudioFrame.h
    AudioOutput.h
    AudioOutputBackend.h
    AudioResample.h
    AudioThread.h
    ByteArray.h
    Demuxer.h
    Frame.h
    OutputSet.h
    Packet.h
    PacketQueue.h
    SubtitleDecoder.h
    VideoDecoder.h
    VideoFormat.h
    VideoFrame.h
    VideoRenderer.h
    VideoThread.h
    decoder/video/VideoDecoderFFmpegBase.h
    framequeue.h
    inner.h
    private/AudioOutputBackend_p.h
    private/AudioResample_p.h
    private/AVDecoder_p.h
    private/AVOutput_p.h
    private/AVThread_p.h
    private/Filter_p.h
    private/Frame_p.h
    private/VideoRender_p.h
    private/player_p.h
    renderer/ColorTransform.h
    renderer/Geometry.h
    renderer/GeometryRenderer.h
    renderer/Matrix4x4.h
    renderer/OpenglAide.h
    renderer/OpenglVideo.h
    renderer/RectF.h
    renderer/ShaderManager.h
    renderer/VectorF.h
    renderer/VideoMaterial.h
    renderer/VideoShader.h
    renderer/VideoShader_p.h
    renderer/glpackage.h
    renderer/subtitlerender.h
    sdk/AVLog.h
    sdk/DPTR.h
    sdk/filter/Filter.h
    sdk/filter/LibAVFilter.h
    sdk/filter/subtitlefilter.h
    sdk/global.h
    sdk/mediainfo.h
    sdk/player.h
    subtitle/Subtitle.h
    subtitle/plaintext.h
    subtitle/subtitledecoder.h
    util/BlockQueue.h
    util/CThread.h
    util/Factory.h
    util/Singleton.h
    util/Size.h
    util/factorybase.h
    util/innermath.h
    util/logsink.h
    util/mkid.h
    util/semaphore.h
    util/stringaide.h)

list(APPEND SOURCES
	AVClock.cpp
	AVDecoder.cpp
	AVDemuxThread.cpp
	AVGlobal.cpp
	AVLog.cpp
	AVOutput.cpp
	AVThread.cpp
	AudioDecoder.cpp
	AudioFormat.cpp
	AudioFrame.cpp
	AudioOutput.cpp
	AudioOutputBackend.cpp
	AudioResample.cpp
	AudioThread.cpp
	ByteArray.cpp
	Demuxer.cpp
	Frame.cpp
	OutputSet.cpp
	Packet.cpp
	PacketQueue.cpp
	Player.cpp
	VideoDecoder.cpp
	VideoFormat.cpp
	VideoFrame.cpp
	VideoRenderer.cpp
	VideoThread.cpp
	decoder/audio/AudioDecoderFFmpeg.cpp
	decoder/video/VideoDecoderFFmpeg.cpp
	decoder/video/VideoDecoderFFmpegBase.cpp
	filter/Filter.cpp
	filter/LibAVFilter.cpp
	filter/subtitlefilter.cpp
	glad/src/glad.c
	output/audio/AudioOutputPortAudio.cpp
	output/audio/audiooutputdsound.cpp
	renderer/ColorTransform.cpp
	renderer/Geometry.cpp
	renderer/GeometryRenderer.cpp
	renderer/Matrix4x4.cpp
	renderer/OpenglAide.cpp
	renderer/OpenglVideo.cpp
	renderer/ShaderManager.cpp
	renderer/VideoMaterial.cpp
	renderer/VideoShader.cpp
	renderer/glpackage.cpp
	renderer/subtitlerender.cpp
	resample/AudioResampleFFmpeg.cpp
	resample/AudioResampleSoundTouch.cpp
	subtitle/Subtitle.cpp
	subtitle/plaintext.cpp
	subtitle/subtitledecoder.cpp
	subtitle/subtitledecoderffmpeg.cpp
	util/CThread.cpp
	util/logsink.cpp)
	
if (EXISTS ${FREETYPE_DIR})
    list(APPEND EXTRA_DEFS -DSMI_HAVE_FREETYPE=1)
	list(APPEND EXTRA_DEFS -DFTGL_LIBRARY_STATIC)
	list(APPEND EXTRA_INCLUDE ${FREETYPE_DIR}/include/freetype2)
	list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/depends/ftgl)
	link_directories(${FREETYPE_DIR}/lib)
endif()
# Include path
list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/depends/soundtouch)
list(APPEND INCLUDE_DIR ${EXTRA_INCLUDE})
include_directories(${INCLUDE_DIR})

link_directories(${CMAKE_SOURCE_DIR}/bin/${CMAKE_INTDIR})

# Output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_INTDIR})
add_library(${MODULE} SHARED ${SOURCES} ${HEADERS})

target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})

#find_library(LIB_AVFORMAT avformat ${FFMPEG_LIB})
list(REMOVE_DUPLICATES EXTRA_LIBS)
target_link_libraries(${MODULE} 
	LINK_PRIVATE ${EXTRA_LIBS} 
	LINK_PRIVATE soundtouch_static
	LINK_PRIVATE opengl32)
if (EXISTS ${FREETYPE_DIR})
	target_link_libraries(${MODULE} 
		LINK_PRIVATE freetype
		LINK_PRIVATE ftgl)
endif()

set_target_properties(${MODULE} PROPERTIES
        VERSION 1.0
        SOVERSION 1.0
        OUTPUT_NAME smi)
