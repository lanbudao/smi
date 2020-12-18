#ifndef INNER_H
#define INNER_H

//TODO: rename to "innerav.h"

#include <memory.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/error.h>
#include <libavutil/attributes.h>
#include "libavutil/version.h"
#ifdef __cplusplus
}
#endif

#define SMI_USE_FFMPEG(MODULE) (MODULE##_VERSION_MICRO >= 100)
#define SMI_USE_LIBAV(MODULE)  !SMI_USE_FFMPEG(MODULE)
#define FFMPEG_MODULE_CHECK(MODULE, MAJOR, MINOR, MICRO) \
    (SMI_USE_FFMPEG(MODULE) && MODULE##_VERSION_INT >= AV_VERSION_INT(MAJOR, MINOR, MICRO))
#define LIBAV_MODULE_CHECK(MODULE, MAJOR, MINOR, MICRO) \
    (SMI_USE_LIBAV(MODULE) && MODULE##_VERSION_INT >= AV_VERSION_INT(MAJOR, MINOR, MICRO))
#define AV_MODULE_CHECK(MODULE, MAJOR, MINOR, MICRO, MINOR2, MICRO2) \
    (LIBAV_MODULE_CHECK(MODULE, MAJOR, MINOR, MICRO) || FFMPEG_MODULE_CHECK(MODULE, MAJOR, MINOR2, MICRO2))

/// example: AV_ENSURE(avcodec_close(avctx), false) will print error and return false if failed. AV_WARN just prints error.
#define AV_ENSURE_OK(FUNC, ...) AV_RUN_CHECK(FUNC, return, __VA_ARGS__)
#define AV_ENSURE(FUNC, ...) AV_RUN_CHECK(FUNC, return, __VA_ARGS__)
#define AV_WARN(FUNC) AV_RUN_CHECK(FUNC, void)

#if AV_MODULE_CHECK(LIBAVUTIL, 55, 0, 0, 0, 100)
#define DESC_VAL(X) (X)
#else
#define DESC_VAL(X) (X##_minus1 + 1)
#endif

#define AVCODEC_STATIC_REGISTER FFMPEG_MODULE_CHECK(LIBAVCODEC, 58, 10, 100)
#define AVFORMAT_STATIC_REGISTER FFMPEG_MODULE_CHECK(LIBAVFORMAT, 58, 9, 100)

#define AV_RUN_CHECK(FUNC, RETURN, ...) do { \
    int ret = FUNC; \
    if (ret < 0) { \
        char str[AV_ERROR_MAX_STRING_SIZE]; \
        memset(str, 0, sizeof(str)); \
        av_strerror(ret, str, sizeof(str)); \
        av_log(NULL, AV_LOG_WARNING, "Error " #FUNC " @%d " __FILE__ ": (%#x) %s\n", __LINE__, ret, str); \
        RETURN __VA_ARGS__; \
     } } while(0)


av_always_inline char* averror2str(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

#endif // INNER_H
