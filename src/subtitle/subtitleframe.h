#ifndef SUBTITLE_FRAME_H
#define SUBTITLE_FRAME_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

typedef struct AVSubtitle AVSubtitle;

NAMESPACE_BEGIN

class SubtitleFramePrivate;
class SubtitleFrame
{
    DPTR_DECLARE_PRIVATE(SubtitleFrame)
public:
    SubtitleFrame();
    SubtitleFrame(const SubtitleFrame &other);
    SubtitleFrame& operator = (const SubtitleFrame &other);

    int serial() const;
    void setSerial(int s);

    bool valid() const { return start < end; }
    bool operator < (const SubtitleFrame &f) const { return end < f.end; }
    inline bool operator < (double t) const { return end < t; }
    double start, end;

    void reset();

    AVSubtitle* data();
    void setData(AVSubtitle *s);

private:
    DPTR_DECLARE(SubtitleFrame)
};

NAMESPACE_END
#endif
