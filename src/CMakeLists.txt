cmake_minimum_required(VERSION 2.8.12)

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
if (ENABLE_CONFIG_TESTS)
    if (HAVE_AVFORMAT)
        find_library(avformat ${FFMPEG_DIR}/lib)
        list(APPEND EXTRA_LIBS avformat)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_AVFORMAT=1")
    endif()
    if (HAVE_AVCODEC)
        list(APPEND EXTRA_LIBS avcodec)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_AVCODEC=1")
    endif()
    if (HAVE_AVFILTER)
        list(APPEND EXTRA_LIBS avfilter)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_AVFILTER=1")
    endif()
    if (HAVE_AVUTIL)
        list(APPEND EXTRA_LIBS avutil)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_AVUTIL=1")
    endif()
    if (HAVE_SWRESAMPLE)
        list(APPEND EXTRA_LIBS swresample)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_SWRESAMPLE=1")
    endif()
    if (HAVE_SWSCALE)
        list(APPEND EXTRA_LIBS swscale)
        list(APPEND EXTRA_DEFS "-DSMI_HAVE_SWSCALE=1")
    endif()
    link_directories(${FFMPEG_DIR}/lib)
    list(APPEND EXTRA_INCLUDE ${FFMPEG_DIR}/include)
else()
    if (EXISTS ${FFMPEG_DIR})
        list(APPEND EXTRA_INCLUDE ${FFMPEG_DIR}/include)
        set(AVMODULES avformat avcodec swscale avutil)
        foreach(av ${AVMODULES})
            string(TOUPPER ${av} AV)
            find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH NO_SYSTEM_ENVIRONMENT_PATH)
            message(STATUS "HAVE_${AV}: ${HAVE_${AV}}")
            if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
                set(HAVE_${AV} 1)
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
            message(STATUS "HAVE_${AV}: ${HAVE_${AV}}")
            if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
                set(HAVE_${AV} 1)
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
    list(APPEND SOURCES output/audio/audiooutputdirectsound.cpp)
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
    sdk/AVLog.h
    sdk/DPTR.h
    sdk/filter/Filter.h
    sdk/filter/LibAVFilter.h
    sdk/global.h
    sdk/mediainfo.h
    sdk/player.h
    sdk/subtitle.h
    subtitle/assrender.h
    subtitle/plaintext.h
    subtitle/SubtitleFrame.h
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
    util/stringaide.h
	io/mediaio.h)

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
	glad/src/glad.c
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
	resample/AudioResampleFFmpeg.cpp
	resample/AudioResampleSoundTouch.cpp
	subtitle/Subtitle.cpp
	subtitle/plaintext.cpp
    subtitle/SubtitleFrame.cpp
	subtitle/subtitledecoder.cpp
	subtitle/subtitledecoderffmpeg.cpp
	util/CThread.cpp
	util/logsink.cpp
    util/semaphore.cpp
	io/mediaio.cpp)
	
#if (EXISTS ${FREETYPE_DIR})
#   list(APPEND EXTRA_DEFS -DSMI_HAVE_FREETYPE=1)
#	list(APPEND EXTRA_DEFS -DFTGL_LIBRARY_STATIC)
#	list(APPEND EXTRA_INCLUDE ${FREETYPE_DIR}/include/freetype2)
#	list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/depends/ftgl)
#	link_directories(${FREETYPE_DIR}/lib)
#endif()
if (EXISTS ${LIBASS_DIR})
    list(APPEND EXTRA_DEFS -DSMI_HAVE_LIBASS=1)
	list(APPEND EXTRA_INCLUDE ${LIBASS_DIR}/include)
	link_directories(${LIBASS_DIR}/lib)
endif()
if (EXISTS ${LIBDVDREAD_DIR})
	if (EXISTS ${LIBDVDNAV_DIR})
		list(APPEND EXTRA_DEFS -DSMI_HAVE_DVD=1)
		list(APPEND SOURCES io/dvdio.cpp)
		list(APPEND EXTRA_INCLUDE ${LIBDVDREAD_DIR}/include)
		list(APPEND EXTRA_INCLUDE ${LIBDVDNAV_DIR}/include)
		link_directories(${LIBDVDREAD_DIR}/lib)
		link_directories(${LIBDVDNAV_DIR}/lib)
	endif()
endif()
# Include path
list(APPEND EXTRA_INCLUDE ${CMAKE_SOURCE_DIR}/depends/soundtouch)
list(APPEND INCLUDE_DIR ${EXTRA_INCLUDE})
include_directories(${INCLUDE_DIR})

link_directories(${CMAKE_SOURCE_DIR}/lib/${CURRENT_PLATFORM})

# Output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/${CURRENT_PLATFORM})
add_library(${MODULE} SHARED ${SOURCES} ${HEADERS})

target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})

#find_library(LIB_AVFORMAT avformat ${FFMPEG_LIB})
list(REMOVE_DUPLICATES EXTRA_LIBS)
target_link_libraries(${MODULE} 
	LINK_PRIVATE ${EXTRA_LIBS} 
	LINK_PRIVATE soundtouch_static)
if (EXISTS ${LIBASS_DIR})
	target_link_libraries(${MODULE} 
		LINK_PRIVATE ass)
endif()
if (EXISTS ${LIBDVDREAD_DIR})
	if (EXISTS ${LIBDVDNAV_DIR})
	target_link_libraries(${MODULE} 
		LINK_PRIVATE dvdread dvdnav)
	endif()
endif()
 
set_target_properties(${MODULE} PROPERTIES
        VERSION 1.0
        SOVERSION 1.0
        OUTPUT_NAME smi
        COMPILE_FLAGS "/wd4819 /wd4996 /wd4018")
