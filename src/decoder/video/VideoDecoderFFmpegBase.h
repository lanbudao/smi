#ifndef VIDEODECODERFFMPEGBASE_H
#define VIDEODECODERFFMPEGBASE_H

#include "VideoDecoder.h"

NAMESPACE_BEGIN

class VideoDecoderFFmpegBasePrivate;
class VideoDecoderFFmpegBase: public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderFFmpegBase)
public:
    VideoDecoderFFmpegBase(VideoDecoderFFmpegBasePrivate *d);
    ~VideoDecoderFFmpegBase();

    virtual int decode(const Packet &pkt);
    VideoFrame frame();

private:

};

NAMESPACE_END
#endif //VIDEODECODERFFMPEGBASE_H
