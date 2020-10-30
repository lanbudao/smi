#ifndef AVOUTPUT_H
#define AVOUTPUT_H

#include "sdk/global.h"
#include "sdk/mediainfo.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

class AVOutputPrivate;
class AVOutput
{
    friend class AVPlayer;
    DPTR_DECLARE_PRIVATE(AVOutput)
public:
    virtual ~AVOutput();

    bool isAvailable() const;
    void setAvailable(bool avaliable);
    void pause(bool p);
    bool isPaused() const;

    virtual void setMediaInfo(MediaInfo *info);

    void setRenderCallback(std::function<void(void* vo_opaque)> cb);

protected:
    AVOutput(AVOutputPrivate *d);
    DPTR_DECLARE(AVOutput)

};

NAMESPACE_END
#endif //AVOUTPUT_H
