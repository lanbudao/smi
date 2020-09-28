#include "sdk/global.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

NAMESPACE_BEGIN

ColorSpace colorSpaceFromFFmpeg(AVColorSpace space)
{
    switch (space)
    {
        case AVCOL_SPC_RGB:
            return ColorSpace_RGB;
        case AVCOL_SPC_BT709:
            return ColorSpace_BT709;
        case AVCOL_SPC_BT470BG:
        case AVCOL_SPC_SMPTE170M:
            return ColorSpace_BT601;
        default:
            return ColorSpace_Unknown;
    }
}

ColorRange colorRangeFromFFmpeg(AVColorRange range)
{
    switch (range)
    {
        case AVCOL_RANGE_JPEG:
            return ColorRange_Full;
        case AVCOL_RANGE_MPEG:
            return ColorRange_Limited;
        default:
            return ColorRange_Unknown;
    }
}

NAMESPACE_END
