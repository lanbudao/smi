#ifndef VIDEODECODERFFMPEGHW_H
#define VIDEODECODERFFMPEGHW_H

#include "VideoDecoderFFmpegBase.h"

NAMESPACE_BEGIN

class VideoDecoderFFmpegHWPrivate;
class VideoDecoderFFmpegHW : public VideoDecoderFFmpegBase
{
public:
    virtual ~VideoDecoderFFmpegHW();

protected:
    VideoDecoderFFmpegHW(VideoDecoderFFmpegHWPrivate* d_ptr = nullptr);

private:
    VideoDecoderFFmpegHW() = default;

};

NAMESPACE_END

#endif //SMI_VIDEODECODERFFMPEGHW_H
