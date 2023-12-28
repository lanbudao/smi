#ifndef MEDIA_IO_P_H
#define MEDIA_IO_P_H

#include "io/mediaio.h"
extern "C" {
#include "libavformat/avformat.h"
}

NAMESPACE_BEGIN

class MediaIOPrivate
{
public:
    MediaIOPrivate():
        ctx(nullptr),
        buffer_size(0),
        mode(MediaIO::Read)
    {

    }
    virtual ~MediaIOPrivate()
    {
    }
    std::string url;
    AVIOContext *ctx;
    int buffer_size;
    MediaIO::AccessMode mode;
};

NAMESPACE_END
#endif
