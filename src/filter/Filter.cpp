#include "sdk/filter/Filter.h"
#include "Filter_p.h"
#include "sdk/player.h"

NAMESPACE_BEGIN

Filter::Filter(FilterPrivate *d):
    d_ptr(d)
{

}

Filter::~Filter() {

}

bool Filter::enabled() const
{
    DPTR_D(const Filter);
    return d->enabled;
}

void Filter::setEnabled(bool b)
{
    DPTR_D(Filter);
    d->enabled = b;
}

Filter::FilterType Filter::type() const
{
    DPTR_D(const Filter);
    return d->type;
}

void Filter::setType(FilterType t)
{
    DPTR_D(Filter);
    d->type = t;
}

VideoFilter::VideoFilter():
    Filter(new VideoFilterPrivate)
{
    setType(Filter::Video);
}

VideoFilter::VideoFilter(VideoFilterPrivate *d):
    Filter(d)
{
    setType(Filter::Video);
}

VideoFilter::~VideoFilter()
{

}

void VideoFilter::installTo(Player *player)
{
    player->installFilter(this);
}

void VideoFilter::apply(MediaInfo * info, VideoFrame * frame)
{
    process(info, frame);
}

AudioFilter::AudioFilter():
    Filter(new AudioFilterPrivate)
{

}

AudioFilter::AudioFilter(AudioFilterPrivate *d):
    Filter(d)
{
    setType(Filter::Audio);
}

AudioFilter::~AudioFilter()
{
    setType(Filter::Audio);
}

void AudioFilter::installTo(Player *player)
{
    player->installFilter(this);
}

void AudioFilter::apply(MediaInfo * info, AudioFrame * frame)
{
    process(info, frame);
}

RenderFilter::RenderFilter() :
    Filter(new RenderFilterPrivate)
{
    setType(Filter::Render);
}

RenderFilter::RenderFilter(RenderFilterPrivate *d) :
    Filter(d)
{
    setType(Filter::Render);
}

RenderFilter::~RenderFilter()
{

}

void RenderFilter::installTo(Player *player)
{
    player->installFilter(this);
}
void RenderFilter::apply(MediaInfo * info, VideoFrame * frame)
{
    process(info, frame);
}
NAMESPACE_END
