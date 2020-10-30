#include "AVOutput.h"
#include "AVOutput_p.h"

NAMESPACE_BEGIN

AVOutput::AVOutput(AVOutputPrivate *d):
    d_ptr(d)
{

}

AVOutput::~AVOutput()
{

}

bool AVOutput::isAvailable() const
{
    return d_func()->available;
}

void AVOutput::setAvailable(bool avail)
{
    DPTR_D(AVOutput);
    d->available = avail;
}

void AVOutput::pause(bool p)
{
    DPTR_D(AVOutput);
    d->pause = p;
}

bool AVOutput::isPaused() const
{
    DPTR_D(const AVOutput);
    return d->pause;
}

void AVOutput::setMediaInfo(MediaInfo *info)
{
    DPTR_D(AVOutput);
    d->media_info = info;
}

void AVOutput::setRenderCallback(std::function<void (void *)> cb)
{
    DPTR_D(AVOutput);
    d->render_cb = cb;
}

NAMESPACE_END
