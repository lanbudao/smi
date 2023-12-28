#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
    explicit Semaphore(int n = 0);

    int available();
    bool tryAcquire(int n = 1);
    void acquire(int n = 1);
    void release(int n = 1);

private:
    std::mutex mutex;
    std::condition_variable cv;
    int avail;
};

#endif //SEMAPHORE_H
