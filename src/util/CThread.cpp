#include "CThread.h"
#include "AVLog.h"

#include <thread>
#include <chrono>
#include <sstream>
#include <string>
#include <atomic>

NAMESPACE_BEGIN

class CThreadPrivate
{
public:
    CThreadPrivate():
        isRunning(false)
    {
    }
    ~CThreadPrivate() {

    }

	std::thread t;
    std::atomic<bool> isRunning;
};

CThread::CThread():
    d_ptr(new CThreadPrivate)
{

}

CThread::~CThread()
{

}

void CThread::start()
{
    DPTR_D(CThread);
	std::thread temp(std::bind(&CThread::run, this));
	d->t = std::move(temp);
	d->isRunning = true;
}

void CThread::stop() {
    DPTR_D(CThread);

    if (!isRunning())
        return;
    stoped();
	AVDebug("Thread %d finished!\n", id());
    d->isRunning = false;
}

void CThread::wait()
{
	DPTR_D(CThread);
	if (d->t.joinable())
		d->t.join();
}

void CThread::run()
{
	DPTR_D(CThread);
	AVDebug("Thread %d finished!\n", id());
	d->isRunning = false;
}

void CThread::stoped()
{

}

void CThread::sleep(int second)
{
    msleep(second * 1000);
}

void CThread::msleep(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

unsigned long CThread::id() const
{
	DPTR_D(const CThread);
	std::thread::id tid = d->t.get_id();
	std::stringstream s;
	s << tid;
#if 0
	_Thrd_t t = *(_Thrd_t*)(char*)&tid;
	return t._Id;
#endif
	return stoul(s.str());
}

bool CThread::isRunning() const {
    DPTR_D(const CThread);
    return d->isRunning;
}

NAMESPACE_END
