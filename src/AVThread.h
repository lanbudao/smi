#ifndef AVTHREAD_H
#define AVTHREAD_H

#include "CThread.h"
#include "mediainfo.h"

NAMESPACE_BEGIN

class AVClock;
class OutputSet;
class PacketQueue;
class AVDecoder;
class Filter;
class AVThreadPrivate;
class AVThread: public CThread
{
    DPTR_DECLARE_PRIVATE(AVThread)
public:
    virtual ~AVThread();

    void setMediaInfo(MediaInfo* info);

    void stop() PU_DECL_OVERRIDE;
    virtual void pause(bool p);

	virtual void requestSeek();
    PacketQueue *packets();

    void setDecoder(AVDecoder *decoder);

    void setOutputSet(OutputSet *output);

	AVClock *clock();
    void setClock(AVClock *clock);

    void executeNextTask();

    bool isSeeking() const;
    void setSeeking(bool s);

    void waitAndCheck(int ms);

    void updateFilters(const std::list<Filter*> filters);
    const std::list<Filter *> &filters() const;

protected:
    AVThread(const char* title, AVThreadPrivate *d);
    DPTR_DECLARE(AVThread)
};

NAMESPACE_END
#endif //AVTHREAD_H
