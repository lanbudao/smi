#ifndef VIDEODECODER_P_H
#define VIDEODECODER_P_H

#include "../../AVDecoder_p.h"

NAMESPACE_BEGIN

class VideoDecoderPrivate: public AVDecoderPrivate
{
public:
    VideoDecoderPrivate() {}
    virtual ~VideoDecoderPrivate() {}
};

NAMESPACE_END

#endif //VIDEODECODER_P_H
