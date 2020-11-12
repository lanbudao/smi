#ifndef AVTHREAD_P_H
#define AVTHREAD_P_H

#include "sdk/DPTR.h"
#include "PacketQueue.h"
#include "AVDecoder.h"
#include "AVClock.h"
#include "Filter.h"
#include "AVLog.h"
#include <shared_mutex>

NAMESPACE_BEGIN


class Runnable {
public:
    Runnable(void *ctx): context(ctx) {}
    virtual void run() = 0;
protected:
    void *context;
};

class OutputSet;
class AVThreadPrivate
{
public:
    AVThreadPrivate() :
        media_info(nullptr),
        decoder(nullptr),
        stopped(false),
        paused(false),
        output(nullptr),
        clock(nullptr),
        seeking(false),
		seek_req(false)
    {
        packets.clear();
    }

    virtual ~AVThreadPrivate() 
    {
        std::unique_lock<std::mutex> lock(mutex);
        filters.clear();
    }

    void addTask(Runnable *task) {
        tasks.push_back(task);
    }

	void waitForRefreshMs(int ms) {
		std::unique_lock<std::mutex> lock(wait_mutex);
		continue_refresh_cond.wait_for(lock, std::chrono::milliseconds(ms));
	}

    MediaInfo *media_info;
    AVDecoder *decoder;
    PacketQueue packets;

    OutputSet *output;

    /*Must be volatile*/
    volatile bool stopped;
    volatile bool paused;

    AVClock *clock;

    std::list<Runnable*> tasks;

    /**
     * Used by video thread when seek is need and media is paused.
     * Then, we need to read a video frame when seek is done
     */
    bool seeking = false;
	bool seek_req;

    /*Filter for audio and video*/
    std::list<Filter*> filters;

	std::mutex mutex;

	std::mutex wait_mutex;
	std::condition_variable continue_refresh_cond;
};

NAMESPACE_END
#endif //AVTHREAD_P_H
