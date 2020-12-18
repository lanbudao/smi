cmake_minimum_required(VERSION 2.8.4)

project(soundtouch)

set(MODULE soundtouch)

# Use C++11
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    message(STATUS "optional:-std=c++11")
#endif(CMAKE_COMPILER_IS_GNUCXX)

list(APPEND EXTRA_DEFS -D__STDC_CONSTANT_MACROS -Wno-dev)

# Load all source file without heareders
# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCES)

list(APPEND SOURCES
    AAFilter.cpp
    BPMDetect.cpp
    cpu_detect_x86.cpp
    FIFOSampleBuffer.cpp
    FIRFilter.cpp
    InterpolateCubic.cpp
    InterpolateLinear.cpp
    InterpolateShannon.cpp
    mmx_optimized.cpp
    PeakFinder.cpp
    RateTransposer.cpp
    SoundTouch.cpp
    sse_optimized.cpp
    TDStretch.cpp)

list(APPEND HEADERS
    AAFilter.h
    BPMDetect.h
    cpu_detect.h
    FIFOSampleBuffer.h
    FIFOSamplePipe.h
    FIRFilter.h
    InterpolateCubic.h
    InterpolateLinear.h
    InterpolateShannon.h
    PeakFinder.h
    RateTransposer.h
    SoundTouch.h
    STTypes.h
    TDStretch.h)

# Output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_INTDIR})
add_library(${MODULE} STATIC ${SOURCES} ${HEADERS})

target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})

set_target_properties(${MODULE} PROPERTIES
        VERSION 1.0
        SOVERSION 1.0
        OUTPUT_NAME soundtouch_static)
