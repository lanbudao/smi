#ifndef FILTER_H
#define FILTER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

class Player;
class FilterPrivate;
class Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    virtual ~Filter();

    bool enabled() const;
    void setEnabled(bool b);

protected:
    Filter(FilterPrivate *d);
    DPTR_DECLARE(Filter)
};

class AudioFilterPrivate;
class AudioFilter: public Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    AudioFilter();
    virtual ~AudioFilter();

    void installTo(Player *player);

protected:
    AudioFilter(AudioFilterPrivate *d);
};

class VideoFilterPrivate;
class VideoFilter: public Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    VideoFilter();
    virtual ~VideoFilter();

    void installTo(Player *player);

protected:
    VideoFilter(VideoFilterPrivate *d);
};

NAMESPACE_END
#endif //FILTER_H
