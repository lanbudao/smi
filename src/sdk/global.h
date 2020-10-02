/******************************************************************************
    FFPRO:  A lightweight player based on ffmpeg and portaudio
    Copyright (C) 2012-2020 lanbudao <theartedly@gmail.com>

*   This file is part of FFPRO

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
******************************************************************************/
#ifndef FF_GLOBAL_H
#define FF_GLOBAL_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <functional>
#include <list>
#include <map>

using namespace std;

#ifdef BUILD_FFPRO_STATIC
#define FFPRO_EXPORT
#else
#ifdef BUILD_FFPRO_LIB
#  undef FFPRO_EXPORT
#  define FFPRO_EXPORT __declspec(dllexport)
#else
#  undef FFPRO_EXPORT
#  define FFPRO_EXPORT __declspec(dllimport)
#endif
#endif
#define PU_AV_PRIVATE_EXPORT FFPRO_EXPORT
#define PU_DECL_DEPRECATED __declspec(deprecated)

#define PU_NO_COPY final
#define PU_DECL_OVERRIDE override
#define PU_SIGNALS public
#define PU_EMIT

#define NAMESPACE_BEGIN namespace FFPRO {
#define NAMESPACE_END }

#define SAFE_DELETE(p) if (p) {delete p; p = nullptr;} 

#ifndef DISABLE_COPY
#define DISABLE_COPY(Class) \
    Class(const Class &) = delete; \
    Class &operator = (const Class &) = delete;
#endif

#define ONLY_RUN_ONES static bool run = false; if (run) return; run = true;

#define PU_UNUSED(X) (void)(X);

#define FORCE_INT(x) static_cast<int>(x)
#define FORCE_UINT(x) static_cast<unsigned int>(x)
#define FORCE_INT64(x) static_cast<int64_t>(x)
#define FORCE_UINT64(x) static_cast<uint64_t>(x)
#define FORCE_FLOAT(x) static_cast<float>(x)
#define FORCE_DOUBLE(x) static_cast<double>(x)

#define FFPRO_MAJOR 1
#define FFPRO_MINOR 0
#define FFPRO_PATCH 0


#define FFPRO_VERSION_MAJOR(V) ((V & 0xff0000) >> 16)
#define FFPRO_VERSION_MINOR(V) ((V & 0xff00) >> 8)
#define FFPRO_VERSION_PATCH(V) (V & 0xff)

#define FFPRO_VERSION_CHK(major, minor, patch) \
    (((major & 0xff) << 16) | ((minor & 0xff) << 8) | (patch & 0xff))

#define FFPRO_VERSION FFPRO_VERSION_CHK(FFPRO_MAJOR, FFPRO_MINOR, FFPRO_PATCH)

#define DECL_LOCKGUARD(mtx) std::lock_guard<std::mutex> lock(mtx); (void)lock;

#define CALL_BACK(FN, ...) if (FN) FN(__VA_ARGS__)

typedef unsigned char uchar;
typedef std::list<std::string> StringList;

NAMESPACE_BEGIN

enum MediaType {
    MediaTypeUnknown = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
    MediaTypeVideo,
    MediaTypeAudio,
    MediaTypeData,          ///< Opaque data information usually continuous
    MediaTypeSubtitle,
    MediaTypeattachment,    ///< Opaque data information usually sparse
    MediaTypeNb
};

enum MediaStatus
{
    NoMedia = 0, // initial status, not invalid. // what if set an empty url and closed?
    Unloaded = 1, // unloaded // (TODO: or when a source(url) is set?)
    Loading = 1<<1, // when source is set
    Loaded = 1<<2, // if auto load and source is set. player is stopped state
    Prepared = 1<<8, // all tracks are buffered and ready to decode frames. tracks failed to open decoder are ignored
    Stalled = 1<<3, // insufficient buffering or other interruptions (timeout, user interrupt)
    Buffering = 1<<4, // NOT IMPLEMENTED
    Buffered = 1<<5, // when playing //NOT IMPLEMENTED
    End = 1<<6, // Playback has reached the end of the current media.
    Seeking = 1<<7, // can be used with Buffering, Loaded. FIXME: NOT IMPLEMENTED
    Invalid = 1<<30, //  invalid media source
};

enum BufferMode: uint8_t {
	BufferTime,
	BufferBytes,
	BufferPackets
};

enum LogLevel: uint8_t {
    LogTrace = 0,
    LogDebug,
    LogInfo,
    LogWarning,
    LogError,
    LogFatal
};

enum ColorSpace {
    ColorSpace_Unknown,
    ColorSpace_RGB,
    ColorSpace_GBR, // for planar gbr format(e.g. video from x264) used in glsl
    ColorSpace_BT601,
    ColorSpace_BT709,
    ColorSpace_XYZ
};

/**
 * @brief The ColorRange enum
 * YUV or RGB color range
 */
enum ColorRange {
    ColorRange_Unknown,
    ColorRange_Limited, // TV, MPEG
    ColorRange_Full     // PC, JPEG
};

enum SeekType {
    SeekFromStart = 1,
    SeekFromNow = 1 << 1,
    SeekAccurate = 1 << 2, // slow
    SeekKeyFrame = 1 << 3, // fast
    /* Use the following combination to seek */
    SeekDefault = SeekFromStart | SeekKeyFrame,
};

enum ClockType
{
    SyncToAudio = 0,
    SyncToVideo,
    SyncToExternalClock,
    SyncToNone = 3 //unused
};

enum ResampleType: uint8_t
{
    ResampleBase,
    ResampleSoundtouch
};

typedef struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0, uint8_t _a = 255) {
        r = _r; g = _g; b = _b; a = _a;
    }
} Color;

NAMESPACE_END
#endif //FF_GLOBAL_H
