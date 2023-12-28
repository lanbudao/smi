#ifndef AVOUTPUT_P_H
#define AVOUTPUT_P_H

#include "sdk/DPTR.h"
#include "Size.h"
#include "sdk/mediainfo.h"
#include "VideoFrame.h"
#include <mutex>
#include <condition_variable>
#include <shared_mutex>

NAMESPACE_BEGIN

class AVOutputPrivate
{
public:
    AVOutputPrivate():
        available(true),
        pause(false),
        media_info(nullptr)
    {

    }
    virtual ~AVOutputPrivate()
    {

    }

    bool available;
    bool pause;
    MediaInfo *media_info;
    VideoFrame current_frame;
    //Callback
    std::function<void(void* vo_opaque)> render_cb = nullptr;
    std::mutex mtx;
    std::condition_variable cond;
};

NAMESPACE_END
#endif //AVOUTPUT_P_H
