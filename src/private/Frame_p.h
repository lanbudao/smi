#ifndef FRAME_P_H
#define FRAME_P_H

#include "sdk/DPTR.h"
#include "sdk/global.h"
#include "ByteArray.h"
#include <vector>
#include <map>
#include <unordered_map>
extern "C" {
#include "libavcodec/avcodec.h"
}

NAMESPACE_BEGIN

class FramePrivate
{
public:
    FramePrivate()
        : timestamp(0),pos(0),
          frame(0),
          serial(-1)
    {
        frame = av_frame_alloc();
    }

    virtual ~FramePrivate()
    {
        if (frame) {
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
    }

    std::vector<uchar *> planes; //slice
    std::vector<int> line_sizes; //stride
    std::map<std::string, std::string> metadata;
    ByteArray data;
    double timestamp;
    int64_t pos;
    AVFrame* frame;
    int serial;
};

NAMESPACE_END
#endif //FRAME_P_H
