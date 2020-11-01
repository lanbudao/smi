#include "AVThread.h"
#include "AVThread_p.h"
#include "AVLog.h"

NAMESPACE_BEGIN

AVThread::AVThread(const char* title, AVThreadPrivate *d):
    CThread(title),
    d_ptr(d)
{

}

AVThread::~AVThread() {

}

void AVThread::stop()
{
    DPTR_D(AVThread);
    if (d->stopped)
		return;
    d->stopped = true;
	d->continue_refresh_cond.notify_all();
	d->packets.setBlock(false);
    d->packets.clear();
    CThread::stop();
}

void AVThread::pause(bool p)
{
    DPTR_D(AVThread);
	d->paused = p;
	d->continue_refresh_cond.notify_all();
}

void AVThread::requestSeek()
{
	DPTR_D(AVThread);
	d->seek_req = true;
	d->continue_refresh_cond.notify_all();
}

PacketQueue * AVThread::packets()
{
    DPTR_D(AVThread);
    return &(d->packets);
}

void AVThread::setDecoder(AVDecoder *decoder)
{
    DPTR_D(AVThread);
    d->decoder = decoder;
}

void AVThread::setOutputSet(OutputSet *output)
{
    DPTR_D(AVThread);
    d->output = output;
}

AVClock *AVThread::clock()
{
	DPTR_D(AVThread);
	return d->clock;
}

void AVThread::setClock(AVClock *clock)
{
    DPTR_D(AVThread);
    d->clock = clock;
}

void AVThread::executeNextTask()
{
    DPTR_D(AVThread);
    if (!d->tasks.empty()) {
        Runnable *task = d->tasks.front();
        d->tasks.pop_front();
        if (task)
            task->run();
        delete task;
    }
}

bool AVThread::isSeeking() const
{
    return d_func()->seeking;
}

void AVThread::setSeeking(bool s)
{
    d_func()->seeking = s;
}

void AVThread::waitAndCheck(int ms)
{
//    AVDebug("wait and check: %d\n", ms);
    if (ms <= 0)
        return;
    int sum = ms;
    while (sum > 0) {
        sum -= 2;
        msleep(2);
        executeNextTask();
    }
}

bool AVThread::installFilter(Filter *filter, int index, bool lock)
{
    DPTR_D(AVThread);
    int p = index;
    if (p < 0)
        p += d->filters.size();
    if (p < 0)
        p = 0;
    if (p > d->filters.size())
        p = d->filters.size();
    std::list<Filter*>::iterator it_dst2;
    std::list<Filter*>::iterator it_dst = d->filters.begin();
    while (--p >= 0) {
        it_dst++;
    }
    std::list<Filter*>::iterator it_src = std::find(d->filters.begin(), d->filters.end(), filter);
    if ((it_dst == it_src) && !d->filters.empty())
        return true;
    if (lock) {
        std::unique_lock<std::mutex> lock_mtx(d->mutex);
        d->filters.remove(filter);
        d->filters.insert(it_dst, filter);
    }
    else {
        d->filters.remove(filter);
        d->filters.insert(it_dst, filter);
    }
    return true;
}

bool AVThread::uninstallFilter(Filter *filter, bool lock)
{
    DPTR_D(AVThread);
    if (lock) {
        std::unique_lock<std::mutex> lock_mtx(d->mutex);
        d->filters.remove(filter);
    }
    else {
        d->filters.remove(filter);
    }
    return true;
}

const std::list<Filter*>& AVThread::filters() const
{
    return d_func()->filters;
}

NAMESPACE_END
