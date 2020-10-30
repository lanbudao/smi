cmake_minimum_required(VERSION 2.8.4)

set(MODULE ffproc)

# Use C++11
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    message(STATUS "optional:-std=c++11")
#endif(CMAKE_COMPILER_IS_GNUCXX)

list(APPEND INCLUDE_DIR .
        sdk
        3dparty/glad/include
        3dparty/SoundTouch
        decoder
        output
        private
        renderer
        filter
        util)

# Check ffmpeg modules
if (EXISTS ${FFMPEG_DIR})
    list(APPEND EXTRA_INCLUDE ${FFMPEG_DIR}/include)
#    link_directories(${FFMPEG_DIR}/lib)
    set(AVMODULES avformat avcodec swscale avutil)
    foreach(av ${AVMODULES})
        string(TOUPPER ${av} AV)
        find_file(HAVE_${AV} lib${av}/${av}.h HINTS ${EXTRA_INCLUDE} PATHS ${EXTRA_INCLUDE} CMAKE_FIND_ROOT_PATH_BOTH)
        message("HAVE_${AV}: ${HAVE_${AV}}")
        if(NOT ${HAVE_${AV}} MATCHES HAVE_${AV}-NOTFOUND)
            set(HAVE_${AV} 1)
#            list(APPEND EXTRA_LIBS ${av})
            find_library(${av}LIB ${av} ${FFMPEG_DIR}/lib)
            list(APPEND EXTRA_LIBS ${${av}LIB})
            list(APPEND EXTRA_DEFS "-DFFPROC_HAVE_${AV}=1")
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
            list(APPEND EXTRA_DEFS "-DFFPROC_HAVE_${AV}=1")
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
    list(APPEND EXTRA_DEFS -DFFPROC_HAVE_PORTAUDIO=1)
    list(APPEND SOURCES output/audio/AudioOutputPortAudio.cpp)
endif()

IF (WIN32)
    list(APPEND EXTRA_DEFS -DFFPROC_HAVE_DSOUND=1)
    list(APPEND SOURCES output/audio/AudioOutputdsound.cpp)
ELSEIF (APPLE)
    MESSAGE(STATUS "Now is Apple systens.")
ELSEIF (UNIX)
    MESSAGE(STATUS "Now is UNIX-like OS's.")
ENDIF()

list(APPEND EXTRA_DEFS -DBUILD_FFPRO_LIB -D__STDC_CONSTANT_MACROS)

#file(GLOB SDK_HEADERS *.h)
file(GLOB SDK_HEADERS_SDK sdk/*.h)
file(GLOB SDK_HEADERS_RENDERER renderer/*.h)
file(GLOB SDK_HEADERS_UTIL util/*.h)
file(GLOB SDK_PRIVATE_HEADERS private/*.h)

# Load all source file without heareders
aux_source_directory(. SOURCES)
aux_source_directory(util SOURCES)
aux_source_directory(filter SOURCES)
aux_source_directory(decoder SOURCES)
aux_source_directory(decoder/audio SOURCES)
aux_source_directory(decoder/video SOURCES)
aux_source_directory(decoder/subtitle SOURCES)
aux_source_directory(3dparty/glad/src SOURCES)
aux_source_directory(3dparty/SoundTouch SOURCES)
aux_source_directory(renderer SOURCES)
aux_source_directory(resample SOURCES)

list(APPEND HEADERS ${SDK_HEADERS}
    ${SDK_PRIVATE_HEADERS}
    ${SDK_HEADERS_SDK}
    ${SDK_HEADERS_UTIL}
    ${SDK_HEADERS_RENDERER})

# Include path
list(APPEND INCLUDE_DIR ${EXTRA_INCLUDE})
include_directories(${INCLUDE_DIR})

# Output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
add_library(${MODULE} SHARED ${SOURCES} ${HEADERS})

target_compile_definitions(${MODULE} PUBLIC ${EXTRA_DEFS})

#find_library(LIB_AVFORMAT avformat ${FFMPEG_LIB})
#find_library(LIB_AVCODEC avcodec ${FFMPEG_LIB})
#find_library(LIB_AVUTIL avutil ${FFMPEG_LIB})
#find_library(LIB_AVSCALE swscale ${FFMPEG_LIB})
#find_library(LIB_AVFILTER avfilter ${FFMPEG_LIB})
#find_library(LIB_AVDEVICE avdevice ${FFMPEG_LIB})
#find_library(LIB_SWRESAMPLE swresample ${FFMPEG_LIB})

target_link_libraries(${MODULE} ${EXTRA_LIBS})
#target_link_libraries(${MODULE} ${LIB_AVFORMAT} ${LIB_AVCODEC} ${LIB_AVUTIL}
#                                ${LIB_AVSCALE} ${LIB_AVFILTER} ${LIB_AVDEVICE}
#                                ${LIB_SWRESAMPLE})

set_target_properties(${MODULE} PROPERTIES
        VERSION 1.0
        SOVERSION 1.0
        OUTPUT_NAME ffproc)
