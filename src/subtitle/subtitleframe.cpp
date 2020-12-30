#include "subtitleframe.h"
extern "C" {
#include <libavcodec/avcodec.h>
}

NAMESPACE_BEGIN

class SubtitleFramePrivate
{
public:
    SubtitleFramePrivate():
        serial(-1)/*,
        subtitle(nullptr)*/
    {
        memset(&subtitle, 0, sizeof(subtitle));
    }
    ~SubtitleFramePrivate()
    {
        avsubtitle_free(&subtitle);
    }

    int serial;
    AVSubtitle subtitle;
};

SubtitleFrame::SubtitleFrame():
    d_ptr(new SubtitleFramePrivate),
    start(0),
    end(0)
{

}

SubtitleFrame::SubtitleFrame(const SubtitleFrame &other):
    d_ptr(other.d_ptr),
    start(other.start),
    end(other.end)
{

}

SubtitleFrame& SubtitleFrame::operator = (const SubtitleFrame &other)
{
    d_ptr = other.d_ptr;
    start = other.start;
    end = other.end;
    return *this;
}

int SubtitleFrame::serial() const
{
    return d_func()->serial;
}

void SubtitleFrame::setSerial(int s)
{
    d_func()->serial = s;
}

void SubtitleFrame::reset()
{
    DPTR_D(SubtitleFrame);
    start = end = 0;
    avsubtitle_free(&d->subtitle);
}

AVSubtitle * SubtitleFrame::data()
{
    DPTR_D(SubtitleFrame);
    return &d->subtitle;
}

void SubtitleFrame::setData(AVSubtitle * s)
{
}

NAMESPACE_END