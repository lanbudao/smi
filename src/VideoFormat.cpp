#include "VideoFormat.h"
#include "inner.h"
#include "innermath.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN


#define FF_HAS_YUV12BITS FFMPEG_MODULE_CHECK(LIBAVUTIL, 51, 73, 101)

static const struct {
    VideoFormat::PixelFormat fmt;
    AVPixelFormat ff;
} pixfmt_map[] = {
    { VideoFormat::Format_YUV420P, AV_PIX_FMT_YUV420P },   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    { VideoFormat::Format_YV12, AV_PIX_FMT_YUV420P },   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    { VideoFormat::Format_YUYV, AV_PIX_FMT_YUYV422 }, //??   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    { VideoFormat::Format_RGB24, AV_PIX_FMT_RGB24 },     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    { VideoFormat::Format_BGR24, AV_PIX_FMT_BGR24 },     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    { VideoFormat::Format_YUV422P, AV_PIX_FMT_YUV422P },   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    { VideoFormat::Format_YUV444P, AV_PIX_FMT_YUV444P },   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    { VideoFormat::Format_YUV410P, AV_PIX_FMT_YUV410P },   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    { VideoFormat::Format_YUV411P, AV_PIX_FMT_YUV411P },   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    { VideoFormat::Format_Y8, AV_PIX_FMT_GRAY8 },     ///<        Y        ,  8bpp
    //AV_PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
    //AV_PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
    //AV_PIX_FMT_PAL8),      ///< 8 bit with PIX_FMT_RGB32 palette
    { VideoFormat::Format_YUV420P, AV_PIX_FMT_YUVJ420P },  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
    //AV_PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range
    //AV_PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range
    //AV_PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing
    //AV_PIX_FMT_XVMC_MPEG2_IDCT,
    { VideoFormat::Format_UYVY, AV_PIX_FMT_UYVY422 },   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    //AV_PIX_FMT_UYYVYY411, ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    //AV_PIX_FMT_BGR8,      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    //AV_PIX_FMT_BGR4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    //AV_PIX_FMT_BGR4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    //AV_PIX_FMT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    //AV_PIX_FMT_RGB4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    //AV_PIX_FMT_RGB4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    { VideoFormat::Format_NV12, AV_PIX_FMT_NV12 },      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    { VideoFormat::Format_NV21, AV_PIX_FMT_NV21 },      ///< as above, but U and V bytes are swapped
    { VideoFormat::Format_ARGB32, AV_PIX_FMT_ARGB },      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    { VideoFormat::Format_RGBA32, AV_PIX_FMT_RGBA },      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    { VideoFormat::Format_ABGR32, AV_PIX_FMT_ABGR },      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    { VideoFormat::Format_BGRA32, AV_PIX_FMT_BGRA },      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    //AV_PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
    { VideoFormat::Format_Y16, AV_PIX_FMT_GRAY16LE },  ///<        Y        , 16bpp, little-endian
    //AV_PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    //AV_PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range
    //AV_PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    /*
    AV_PIX_FMT_VDPAU_H264,///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_MPEG1,///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_MPEG2,///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_WMV3,///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_VDPAU_VC1, ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    */
    { VideoFormat::Format_RGB48BE, AV_PIX_FMT_RGB48BE },   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
    { VideoFormat::Format_RGB48LE, AV_PIX_FMT_RGB48LE },   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian
    { VideoFormat::Format_RGB565, AV_PIX_FMT_RGB565 },  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), native-endian
    { VideoFormat::Format_RGB555, AV_PIX_FMT_RGB555 },  ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), native-endian, be: most significant bit to 1
    { VideoFormat::Format_BGR565, AV_PIX_FMT_BGR565 },  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), native-endian
    { VideoFormat::Format_BGR555, AV_PIX_FMT_BGR555 },  ///< packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), native-endian, be: most significant bit to 1
/*
    AV_PIX_FMT_VAAPI_MOCO, ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
    AV_PIX_FMT_VAAPI_IDCT, ///< HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers
    AV_PIX_FMT_VAAPI_VLD,  ///< HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
*/
#if FF_HAS_YUV12BITS
    { VideoFormat::Format_YUV420P16LE, AV_PIX_FMT_YUV420P16LE },  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    { VideoFormat::Format_YUV420P16BE, AV_PIX_FMT_YUV420P16BE },  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    { VideoFormat::Format_YUV422P16LE, AV_PIX_FMT_YUV422P16LE },  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    { VideoFormat::Format_YUV422P16BE, AV_PIX_FMT_YUV422P16BE },  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    { VideoFormat::Format_YUV444P16LE, AV_PIX_FMT_YUV444P16LE },  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    { VideoFormat::Format_YUV444P16BE, AV_PIX_FMT_YUV444P16BE },  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
#endif //FF_HAS_YUV12BITS
/*
    AV_PIX_FMT_VDPAU_MPEG4,  ///< MPEG4 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    AV_PIX_FMT_DXVA2_VLD,    ///< HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

    AV_PIX_FMT_RGB444LE,  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian, most significant bits to 0
    AV_PIX_FMT_RGB444BE,  ///< packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian, most significant bits to 0
    AV_PIX_FMT_BGR444LE,  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian, most significant bits to 1
    AV_PIX_FMT_BGR444BE,  ///< packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian, most significant bits to 1
    AV_PIX_FMT_GRAY8A,    ///< 8bit gray, 8bit alpha
    */
    { VideoFormat::Format_BGR48BE, AV_PIX_FMT_BGR48BE },   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian
    { VideoFormat::Format_BGR48LE, AV_PIX_FMT_BGR48LE },   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as little-endian
    //the following 10 formats have the disadvantage of needing 1 format for each bit depth, thus
    //If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately
    //is better
    // the ffmpeg QtAV can build against( >= 0.9) supports 9,10 bits
    { VideoFormat::Format_YUV420P9BE, AV_PIX_FMT_YUV420P9BE }, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    { VideoFormat::Format_YUV420P9LE, AV_PIX_FMT_YUV420P9LE }, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    { VideoFormat::Format_YUV420P10BE, AV_PIX_FMT_YUV420P10BE },///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    { VideoFormat::Format_YUV420P10LE, AV_PIX_FMT_YUV420P10LE },///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    { VideoFormat::Format_YUV422P10BE, AV_PIX_FMT_YUV422P10BE },///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    { VideoFormat::Format_YUV422P10LE, AV_PIX_FMT_YUV422P10LE },///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    { VideoFormat::Format_YUV444P9BE, AV_PIX_FMT_YUV444P9BE }, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    { VideoFormat::Format_YUV444P9LE, AV_PIX_FMT_YUV444P9LE }, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    { VideoFormat::Format_YUV444P10BE, AV_PIX_FMT_YUV444P10BE },///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    { VideoFormat::Format_YUV444P10LE, AV_PIX_FMT_YUV444P10LE },///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    { VideoFormat::Format_YUV422P9BE, AV_PIX_FMT_YUV422P9BE }, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    { VideoFormat::Format_YUV422P9LE, AV_PIX_FMT_YUV422P9LE }, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    //AV_PIX_FMT_VDA_VLD,    ///< hardware decoding through VDA
/*
#ifdef AV_PIX_FMT_ABI_GIT_MASTER
    AV_PIX_FMT_RGBA64BE },  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_RGBA64LE },  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
    AV_PIX_FMT_BGRA64BE },  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    AV_PIX_FMT_BGRA64LE },  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
#endif
    AV_PIX_FMT_GBRP,      ///< planar GBR 4:4:4 24bpp
    AV_PIX_FMT_GBRP9BE,   ///< planar GBR 4:4:4 27bpp, big-endian
    AV_PIX_FMT_GBRP9LE,   ///< planar GBR 4:4:4 27bpp, little-endian
    AV_PIX_FMT_GBRP10BE,  ///< planar GBR 4:4:4 30bpp, big-endian
    AV_PIX_FMT_GBRP10LE,  ///< planar GBR 4:4:4 30bpp, little-endian
    AV_PIX_FMT_GBRP16BE,  ///< planar GBR 4:4:4 48bpp, big-endian
    AV_PIX_FMT_GBRP16LE,  ///< planar GBR 4:4:4 48bpp, little-endian
*/
    /**
     * duplicated pixel formats for compatibility with libav.
     * FFmpeg supports these formats since May 8 2012 and Jan 28 2012 (commits f9ca1ac7 and 143a5c55)
     * Libav added them Oct 12 2012 with incompatible values (commit 6d5600e85)
     */
/*
    AV_PIX_FMT_YUVA422P_LIBAV,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    AV_PIX_FMT_YUVA444P_LIBAV,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)

    AV_PIX_FMT_YUVA420P9BE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), big-endian
    AV_PIX_FMT_YUVA420P9LE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), little-endian
    AV_PIX_FMT_YUVA422P9BE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), big-endian
    AV_PIX_FMT_YUVA422P9LE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), little-endian
    AV_PIX_FMT_YUVA444P9BE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), big-endian
    AV_PIX_FMT_YUVA444P9LE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), little-endian
    // Y: 2bytes/pix U: 1, V: 1
    AV_PIX_FMT_YUVA420P10BE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA420P10LE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA422P10BE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA422P10LE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA444P10BE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA444P10LE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA420P16BE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA420P16LE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA422P16BE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA422P16LE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
    AV_PIX_FMT_YUVA444P16BE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
    AV_PIX_FMT_YUVA444P16LE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)
*/
    //AV_PIX_FMT_VDPAU,     ///< HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface
    // doc/APIChanges: 2014-04-07 - 0a1cc04 / 8b17243 - lavu 52.75.100 / 53.11.0 - pixfmt.h
    //Add AV_PIX_FMT_YVYU422 pixel format.
#if (FFMPEG_MODULE_CHECK(LIBAVUTIL, 52, 75, 100) || LIBAV_MODULE_CHECK(LIBAVUTIL, 53, 11, 0))
    { VideoFormat::Format_YVYU, AV_PIX_FMT_YVYU422 },
#endif
// 2014-03-16 - 6b1ca17 / 1481d24 - lavu 52.67.100 / 53.6.0 before ffmpeg2.2 libav11 RGBA64_LIBAV
#if (SMI_USE_FFMPEG(LIBAVUTIL) || LIBAV_MODULE_CHECK(LIBAVUTIL, 53, 6, 0))
    { VideoFormat::Format_RGBA64BE, AV_PIX_FMT_RGBA64BE },  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    { VideoFormat::Format_RGBA64LE, AV_PIX_FMT_RGBA64LE },  ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
    { VideoFormat::Format_BGRA64BE, AV_PIX_FMT_BGRA64BE },  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
    { VideoFormat::Format_BGRA64LE, AV_PIX_FMT_BGRA64LE },  ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
#endif
#if SMI_USE_FFMPEG(LIBAVUTIL) //still use rgba formats but check hasAplha is required
    { VideoFormat::Format_ARGB32, AV_PIX_FMT_0RGB },      ///< packed RGB 8:8:8, 32bpp, 0RGB0RGB...
    { VideoFormat::Format_RGBA32, AV_PIX_FMT_RGB0 },      ///< packed RGB 8:8:8, 32bpp, RGB0RGB0...
    { VideoFormat::Format_ABGR32, AV_PIX_FMT_0BGR },      ///< packed BGR 8:8:8, 32bpp, 0BGR0BGR...
    { VideoFormat::Format_BGRA32, AV_PIX_FMT_BGR0 },      ///< packed BGR 8:8:8, 32bpp, BGR0BGR0...
#endif //
    //AV_PIX_FMT_YUVA444P,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    //AV_PIX_FMT_YUVA422P,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)

#if FF_HAS_YUV12BITS
    { VideoFormat::Format_YUV420P12BE, AV_PIX_FMT_YUV420P12BE }, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    { VideoFormat::Format_YUV420P12LE, AV_PIX_FMT_YUV420P12LE }, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    { VideoFormat::Format_YUV420P14BE, AV_PIX_FMT_YUV420P14BE }, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
    { VideoFormat::Format_YUV420P14LE, AV_PIX_FMT_YUV420P14LE }, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
    { VideoFormat::Format_YUV422P12BE, AV_PIX_FMT_YUV422P12BE }, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    { VideoFormat::Format_YUV422P12LE, AV_PIX_FMT_YUV422P12LE }, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    { VideoFormat::Format_YUV422P14BE, AV_PIX_FMT_YUV422P14BE }, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
    { VideoFormat::Format_YUV422P14LE, AV_PIX_FMT_YUV422P14LE }, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
    { VideoFormat::Format_YUV444P12BE, AV_PIX_FMT_YUV444P12BE }, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    { VideoFormat::Format_YUV444P12LE, AV_PIX_FMT_YUV444P12LE }, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
    { VideoFormat::Format_YUV444P14BE, AV_PIX_FMT_YUV444P14BE }, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
    { VideoFormat::Format_YUV444P14LE, AV_PIX_FMT_YUV444P14LE }, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
#endif //FF_HAS_YUV12BITS
/*
    AV_PIX_FMT_GBRP12BE,    ///< planar GBR 4:4:4 36bpp, big-endian
    AV_PIX_FMT_GBRP12LE,    ///< planar GBR 4:4:4 36bpp, little-endian
    AV_PIX_FMT_GBRP14BE,    ///< planar GBR 4:4:4 42bpp, big-endian
    AV_PIX_FMT_GBRP14LE,    ///< planar GBR 4:4:4 42bpp, little-endian
*/
    // native endian formats
    // AV_PIX_FMT_RGB32) is depends on byte order, ARGB for BE, BGRA for LE
    { VideoFormat::Format_RGB32, AV_PIX_FMT_RGB32 }, //auto endian
    // AV_PIX_FMT_BGR32_1: bgra, argb
    { VideoFormat::Format_BGR32, AV_PIX_FMT_BGR32 }, //auto endian
    { VideoFormat::Format_RGB48, AV_PIX_FMT_RGB48 },   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
    { VideoFormat::Format_BGR48, AV_PIX_FMT_BGR48 },   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian
#if SMI_USE_FFMPEG(LIBAVUTIL)
    { VideoFormat::Format_RGBA64, AV_PIX_FMT_RGBA64 },
    { VideoFormat::Format_BGRA64, AV_PIX_FMT_BGRA64 },
#endif //SMI_USE_FFMPEG(LIBAVUTIL)
    { VideoFormat::Format_VYUY, AV_PIX_FMT_UYVY422 }, // FIXME: hack for invalid ffmpeg formats

    { VideoFormat::Format_VYU, AV_PIX_FMT_RGB32 },
#ifdef AV_PIX_FMT_XYZ12
    { VideoFormat::Format_XYZ12, AV_PIX_FMT_XYZ12 },
    { VideoFormat::Format_XYZ12LE, AV_PIX_FMT_XYZ12LE },
    { VideoFormat::Format_XYZ12BE, AV_PIX_FMT_XYZ12BE },
#endif
    { VideoFormat::Format_Invalid, AV_PIX_FMT_NONE },
};

class VideoFormatPrivate/*: public DptrPrivate<VideoFormat>*/
{
public:
    VideoFormatPrivate():
        pixfmt(VideoFormat::Format_Invalid),
        pixfmt_ff(AV_PIX_FMT_NONE),
        descriptor(nullptr),
        bitsPerPixel(0),
        bitsPerComponent(0),
        bitsPerPixel_pad(0)
    {

    }

    ~VideoFormatPrivate()
    {
        descriptor = nullptr;
    }

    void setFormat(VideoFormat::PixelFormat fmt)
    {
        pixfmt = fmt;
        pixfmt_ff = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg(fmt);
        init();
    }

    void setFormat(AVPixelFormat ff)
    {
        pixfmt_ff = ff;
        pixfmt = (VideoFormat::PixelFormat)VideoFormat::pixelFormatFromFFmpeg(ff);
        init();
    }

    void init()
    {
        if (pixfmt_ff == AV_PIX_FMT_NONE)
            return;
        planes = (uint8_t)std::max(av_pix_fmt_count_planes(pixfmt_ff), 0);
        bitsPerPixels.reserve(planes);
        bitsPerPixels.resize(planes);

        channels.reserve(planes);
        channels.resize(planes);

        descriptor = const_cast<AVPixFmtDescriptor*>(av_pix_fmt_desc_get(pixfmt_ff));
        if (!descriptor)
            return;
        initBitsPerPixel();
    }

    uint64_t flags() const {
        if (!descriptor)
            return 0;
        return descriptor->flags;
    }

    int bytesPerLine(int width, int plane) const {
        return av_image_get_linesize(pixfmt_ff, width, plane);
    }

    std::string name() const {
        if (pixfmt_ff == AV_PIX_FMT_NONE)
            return std::string("");
        return av_get_pix_fmt_name(pixfmt_ff);
    }

    VideoFormat::PixelFormat pixfmt;
    AVPixelFormat pixfmt_ff;
    AVPixFmtDescriptor *descriptor;

    uint8_t planes;
    std::vector<int> bitsPerPixels;
    std::vector<int> channels;
    uint8_t bitsPerPixel;
    uint8_t bitsPerPixel_pad;
    uint8_t bitsPerComponent;

private:
    // from libavutil/pixdesc.c
    void initBitsPerPixel()
    {
        bitsPerPixel = bitsPerPixel_pad = 0;
        bitsPerComponent = DESC_VAL(descriptor->comp[0].depth);
        const int log2_pixels = descriptor->log2_chroma_w + descriptor->log2_chroma_h;
        int steps[4];
        memset(steps, 0, sizeof(steps));
        for (int i = 0; i < descriptor->nb_components; ++i) {
            const AVComponentDescriptor *des = &descriptor->comp[i];
            int s = (i == 1 || i == 2) ? 0 : log2_pixels;
            bitsPerPixels[des->plane] += DESC_VAL(des->depth);
            steps[des->plane] = DESC_VAL(des->step) << s;
            channels[des->plane] += 1;
            bitsPerPixel += (DESC_VAL(des->depth) << s);
            if (DESC_VAL(des->depth) != bitsPerComponent)
                bitsPerComponent = 0;
        }
        for (int i = 0; i < planes; ++i) {
            bitsPerPixel_pad += steps[i];
        }
        if (!(descriptor->flags & AV_PIX_FMT_FLAG_BITSTREAM)) {
            bitsPerPixel_pad *= 8;
        }
        bitsPerPixel >>= log2_pixels;
        bitsPerPixel_pad >>= log2_pixels;
    }
};

VideoFormat::VideoFormat(PixelFormat fmt):
    d(new VideoFormatPrivate)
{
//    //DPTR_D(VideoFormat);
    d->setFormat(fmt);
}

VideoFormat::VideoFormat(int pix_fmt):
    d(new VideoFormatPrivate)
{
//    //DPTR_D(VideoFormat);
    d->setFormat((AVPixelFormat)pix_fmt);
}

VideoFormat::VideoFormat(const VideoFormat &other)
{
//    dptr_d = other.dptr_d;
    d = other.d;
}

VideoFormat &VideoFormat::operator=(const VideoFormat &other)
{
//    dptr_d = other.dptr_d;
    d = other.d;
    return *this;
}

VideoFormat::~VideoFormat() {

}

VideoFormat::PixelFormat VideoFormat::pixelFormatFromFFmpeg(int ff) {
    for (int i = 0; i < sizeof(pixfmt_map) / sizeof(pixfmt_map[0]); ++i) {
        if (pixfmt_map[i].ff == ff)
            return pixfmt_map[i].fmt;
    }
    return Format_Invalid;
}

int VideoFormat::pixelFormatToFFmpeg(VideoFormat::PixelFormat fmt) {
    for (int i = 0; i < sizeof(pixfmt_map) / sizeof(pixfmt_map[0]); ++i) {
        if (pixfmt_map[i].fmt == fmt)
            return pixfmt_map[i].ff;
    }
    return AV_PIX_FMT_NONE;
}

std::vector<int> VideoFormat::pixelFormatsFFmpeg() {
    std::vector<int> fmts;
    if (fmts.empty()) {
        const AVPixFmtDescriptor *descriptor = nullptr;
        while ( (descriptor = av_pix_fmt_desc_next(descriptor)) ) {
            fmts.push_back(av_pix_fmt_desc_get_id(descriptor));
        }
    }
    return fmts;
}

int VideoFormat::bytesPerLine(int width, int plane) const {
    //DPTR_D(const VideoFormat);
    return d->bytesPerLine(width, plane);
}

int VideoFormat::chromaWidth(int lumaWidth) const {
    //DPTR_D(const VideoFormat);
    return -((-lumaWidth) >> d->descriptor->log2_chroma_w);
}

int VideoFormat::chromaHeight(int lumaHeight) const {
    //DPTR_D(const VideoFormat);
    return -((-lumaHeight) >> d->descriptor->log2_chroma_h);
}

int VideoFormat::width(int lumaWidth, int plane) const {
    //DPTR_D(const VideoFormat);
    if (plane <= 0)
        return lumaWidth;
    return chromaWidth(lumaWidth);
}

int VideoFormat::height(int lumaHeight, int plane) const {
    //DPTR_D(const VideoFormat);
    if (plane <= 0)
        return lumaHeight;
    return chromaHeight(lumaHeight);
}

double VideoFormat::normalizedWidth(int plane) const
{
	if (plane <= 0)
		return 1.0;
	return 1.0 / std::pow(2.0, double(d->descriptor->log2_chroma_w));
}

double VideoFormat::normalizedHeight(int plane) const
{
	if (plane <= 0)
		return 1.0;
	return 1.0 / std::pow(2.0, double(d->descriptor->log2_chroma_h));
}

std::string VideoFormat::name() const {
    //DPTR_D(const VideoFormat);
    return d->name();
}

bool VideoFormat::isBigEndian() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_BE) == AV_PIX_FMT_FLAG_BE;
}

bool VideoFormat::isBitStream() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_BITSTREAM) == AV_PIX_FMT_FLAG_BITSTREAM;
}

bool VideoFormat::isHwaccel() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_HWACCEL) == AV_PIX_FMT_FLAG_HWACCEL;
}

bool VideoFormat::isPlanar() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_PLANAR) == AV_PIX_FMT_FLAG_PLANAR;
}

bool VideoFormat::isRGB() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB &&
            d->pixfmt != Format_VYU;
}

bool VideoFormat::isXYZ() const {
    //DPTR_D(const VideoFormat);
    return d->pixfmt == Format_XYZ12 || d->pixfmt == Format_XYZ12BE || d->pixfmt == Format_XYZ12LE;
}

bool VideoFormat::hasAlpha() const {
    //DPTR_D(const VideoFormat);
    return (d->flags() & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;
}

bool VideoFormat::isPlanar(PixelFormat pixfmt) {
    return pixfmt == Format_YUV420P || pixfmt == Format_NV12 || pixfmt == Format_NV21 ||
           pixfmt == Format_YV12 || pixfmt == Format_YUV410P || pixfmt == Format_YUV411P ||
           pixfmt == Format_YUV422P || pixfmt == Format_YUV444P || pixfmt == Format_AYUV444 ||
           pixfmt == Format_IMC1 || pixfmt == Format_IMC2 || pixfmt == Format_IMC3 ||
           pixfmt == Format_IMC4;
}

bool VideoFormat::isRGB(PixelFormat pixfmt) {
    return pixfmt == Format_RGB32 || pixfmt == Format_ARGB32 || pixfmt == Format_RGB24 ||
           pixfmt == Format_BGRA32 || pixfmt == Format_ABGR32 || pixfmt == Format_RGBA32 ||
           pixfmt == Format_BGR565 || pixfmt == Format_RGB555 || pixfmt == Format_RGB565 ||
           pixfmt == Format_BGR24 || pixfmt == Format_BGR32 || pixfmt == Format_BGR555 ||
           pixfmt == Format_RGB48 || pixfmt == Format_RGB48LE || pixfmt == Format_RGB48BE ||
           pixfmt == Format_BGR48 || pixfmt == Format_BGR48LE || pixfmt == Format_BGR48BE ||
           pixfmt == Format_RGBA64 || pixfmt == Format_RGBA64LE || pixfmt == Format_RGBA64BE ||
           pixfmt == Format_BGRA64 || pixfmt == Format_BGRA64LE || pixfmt == Format_BGRA64BE;
}

bool VideoFormat::hasAlpha(PixelFormat pixfmt) {
    return pixfmt == Format_ABGR32 || pixfmt == Format_ARGB32 || pixfmt == Format_AYUV444;
}

int VideoFormat::channels() const {
    //DPTR_D(const VideoFormat);
    if (!d->descriptor)
        return 0;
    return d->descriptor->nb_components;
}

int VideoFormat::channels(int plane) const {
    //DPTR_D(const VideoFormat);
    if (plane >= d->channels.size() || plane < 0)
        return 0;
    return d->channels.at(plane);
}

unsigned int VideoFormat::planeCount() const {
    //DPTR_D(const VideoFormat);
    return d->planes;
}

int VideoFormat::bitsPerPixel() const {
    //DPTR_D(const VideoFormat);
    return d->bitsPerPixel;
}

int VideoFormat::bitsPerPixelPadded() const {
    //DPTR_D(const VideoFormat);
    return d->bitsPerPixel_pad;
}

int VideoFormat::bitsPerPixel(int plane) const {
    //DPTR_D(const VideoFormat);
    if (plane >= d->bitsPerPixels.size() || plane < 0)
        return 0;
    return d->bitsPerPixels.at(plane);
}

int VideoFormat::bitsPerComponent() const {
    //DPTR_D(const VideoFormat);
    return d->bitsPerComponent;
}

int VideoFormat::bytesPerPixel() const {
    //DPTR_D(const VideoFormat);
    return (d->bitsPerPixel + 7) >> 3;
}

int VideoFormat::bytesPerPixel(int plane) const {
    //DPTR_D(const VideoFormat);
    if (plane >= d->bitsPerPixels.size() || plane < 0)
        return 0;
    return d->bitsPerPixels.at(plane);
}

bool VideoFormat::isValid() const {
    //DPTR_D(const VideoFormat);
    return d->pixfmt != Format_Invalid || d->pixfmt_ff != AV_PIX_FMT_NONE;
}

VideoFormat::PixelFormat VideoFormat::pixelFormat() const {
    //DPTR_D(const VideoFormat);
    return d->pixfmt;
}

int VideoFormat::pixelFormatFFmpeg() const {
    //DPTR_D(const VideoFormat);
    return d->pixfmt_ff;
}

bool VideoFormat::operator==(const VideoFormat &other) const
{
	return d->pixfmt_ff == other.d->pixfmt_ff;
}

bool VideoFormat::operator!=(const VideoFormat& other) const
{
	return !(*this == other);
}

NAMESPACE_END
