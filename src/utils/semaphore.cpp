#include "semaphore.h"

Semaphore::Semaphore(int n):
    avail(n)
{
}

bool Semaphore::tryAcquire(int n)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (n > avail)
        return false;
    avail -= n;
    return true;
}

int Semaphore::available()
{
    std::unique_lock<std::mutex> lock(mutex);
    return avail;
}

void Semaphore::acquire(int n)
{
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [=] {return avail > 0; });
    avail -= n;
}

void Semaphore::release(int n)
{
    std::unique_lock<std::mutex> lock(mutex);
    avail += n;
    cv.notify_all();
}
