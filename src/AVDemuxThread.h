#ifndef AVDEMUXTHREAD_H
#define AVDEMUXTHREAD_H

#include "CThread.h"

NAMESPACE_BEGIN

class AVClock;
class AVThread;
class Demuxer;
class AVDemuxThreadPrivate;
class AVDemuxThread: public CThread
{
    DPTR_DECLARE_PRIVATE(AVDemuxThread)
public:
    AVDemuxThread();
    ~AVDemuxThread() PU_DECL_OVERRIDE;

    void stop() PU_DECL_OVERRIDE;
    void pause(bool p);
    void seek(double pos, double rel, SeekType type);
    void setDemuxer(Demuxer *demuxer);
    void setAudioThread(AVThread *thread);
    AVThread *audioThread();
    void setVideoThread(AVThread *thread);
    AVThread *videoThread();
    void setClock(AVClock *clock);
	void stepToNextFrame();
    void updateBufferStatus();

    void setMediaStatusChangedCB(std::function<void(MediaStatus s)> f);
    void setBufferProcessChangedCB(std::function<void(float p)> f);

protected:
    void run() PU_DECL_OVERRIDE;

private:
    DPTR_DECLARE(AVDemuxThread)

};

NAMESPACE_END
#endif //AVDEMUXTHREAD_H
