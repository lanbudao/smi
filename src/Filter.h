#ifndef FILTER_H
#define FILTER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "sdk/mediainfo.h"

NAMESPACE_BEGIN

class Player;
class AudioFrame;
class VideoFrame;
class FilterPrivate;
class FFPRO_EXPORT Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    virtual ~Filter();

    bool enabled() const;
    void setEnabled(bool b);

protected:
    Filter(FilterPrivate *d);
    class FFPRO_EXPORT DPTR_DECLARE(Filter)
};

class AudioFilterPrivate;
class FFPRO_EXPORT AudioFilter: public Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    AudioFilter();
    virtual ~AudioFilter();

    void installTo(Player *player);
    void apply(MediaInfo* info, AudioFrame *frame = 0);

protected:
    AudioFilter(AudioFilterPrivate *d);
    virtual bool process(MediaInfo* info, AudioFrame* frame = 0) = 0;
};

class VideoFilterPrivate;
class FFPRO_EXPORT VideoFilter: public Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    VideoFilter();
    virtual ~VideoFilter();

    void installTo(Player *player);
    void apply(MediaInfo* info, VideoFrame *frame = 0);

protected:
    VideoFilter(VideoFilterPrivate *d);
    virtual bool process(MediaInfo* info, VideoFrame* frame = 0) = 0;
};

class RenderFilterPrivate;
class FFPRO_EXPORT RenderFilter : public Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    RenderFilter();
    virtual ~RenderFilter();

    void installTo(Player *player);

protected:
    RenderFilter(RenderFilterPrivate *d);
    virtual void process(MediaInfo* info, VideoFrame* frame = 0) = 0;
};
NAMESPACE_END
#endif //FILTER_H
