#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
    explicit Semaphore(int n = 0): avail(n) {}

    int available();
    bool tryAcquire(int n = 1);
    void acquire(int n = 1);
    void release(int n = 1);

private:
    std::mutex mutex;
    std::condition_variable cv;
    int avail;
};

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

#endif //SEMAPHORE_H
