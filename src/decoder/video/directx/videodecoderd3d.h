#ifndef VIDEODECODERD3D_H
#define VIDEODECODERD3D_H

#include "../VideoDecoderFFmpegHW.h"

NAMESPACE_BEGIN

class VideoDecoderD3DPrivate;
class VideoDecoderD3D: public VideoDecoderFFmpegHW
{
public:
    VideoDecoderD3D(VideoDecoderD3DPrivate *d_ptr = nullptr);

};

NAMESPACE_END
#endif // VIDEODECODERD3D_H
