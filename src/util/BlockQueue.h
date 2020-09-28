#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <queue>
#include <condition_variable>
#include <shared_mutex>
#include "sdk/global.h"

NAMESPACE_BEGIN

template <typename T>
class BlockQueue
{
public:
    BlockQueue();
    virtual ~BlockQueue() {}

    /**
     * @brief enqueue
     * @param timeout wait time out(ms)
     */
    void enqueue(const T &t, unsigned long timeout = ULONG_MAX);
    T dequeue(bool *isValid = nullptr, unsigned long timeout = ULONG_MAX);

    void clear();
    void setCapacity(int cap);
    void setThreshold(int thr);
    void setBlock(bool block);
    bool isEmpty() const;
    bool isEnough() const;
    void blockEmpty(bool block);
    void blockFull(bool block);
    unsigned int size() const;

protected:
    virtual bool checkFull() const;
    virtual bool checkEmpty() const;
    virtual bool checkEnough() const;

    virtual void onEnqueue(const T &t) {}
    virtual void onDequeue(const T &t) {}

protected:
    std::queue<T> q;

    /*Must be mutable*/
    mutable std::mutex mutex;
    std::mutex lock_change_mutex;
    std::condition_variable empty_cond, full_cond;

    int capacity, threshold;
    bool block_full, block_empty;
};

template<typename T>
BlockQueue<T>::BlockQueue():
        block_full(true),
        block_empty(true),
        capacity(48),
        threshold(32)
{

}

template<typename T>
void BlockQueue<T>::clear()
{
    std::unique_lock<std::mutex> lock(mutex);
    full_cond.notify_all();
	empty_cond.notify_all();

    /*Clear the queue*/
    std::queue<T> null;
    std::swap(q, null);
    onDequeue(T());
}

template<typename T>
void BlockQueue<T>::enqueue(const T &t, unsigned long timeout)
{
    std::unique_lock<std::mutex> lock(mutex);

    if (checkFull()) {
        if (block_full) {
            full_cond.wait_for(lock, std::chrono::milliseconds(timeout));
        }
	}
	onEnqueue(t);
    q.push(t);

    if (checkEnough()) {
        empty_cond.notify_one();
    }
}

template<typename T>
T BlockQueue<T>::dequeue(bool *isValid, unsigned long timeout)
{
    if (isValid)
        *isValid = false;

    std::unique_lock<std::mutex> lock(mutex);
    if (checkEmpty()) {
        if (block_empty)
            empty_cond.wait_for(lock, std::chrono::milliseconds(timeout));
    }
    if (checkEmpty())
        return T();

    T t = q.front();
    q.pop();
	full_cond.notify_one();
	onDequeue(t);

    if (isValid)
        *isValid = true;
    return t;
}

template<typename T>
void BlockQueue<T>::setBlock(bool block)
{
    std::unique_lock<std::mutex> lock(mutex);
    block_full = block_empty = block;
    if (!block) {
        full_cond.notify_all();
        empty_cond.notify_all();
    }
}

template<typename T>
void BlockQueue<T>::setCapacity(int cap) {
    capacity = cap;
}

template<typename T>
void BlockQueue<T>::setThreshold(int thr) {
    threshold = thr;
}

template<typename T>
bool BlockQueue<T>::isEmpty() const {
    return q.empty();
}

template<typename T>
bool BlockQueue<T>::isEnough() const {
    return q.size() >= (unsigned int)threshold;
}

template<typename T>
bool BlockQueue<T>::checkFull() const {
    return q.size() >= (unsigned int)capacity;
}

template<typename T>
bool BlockQueue<T>::checkEmpty() const {
    return q.empty();
}

template<typename T>
bool BlockQueue<T>::checkEnough() const {
    return q.size() >= (unsigned int)threshold;
}

template<typename T>
void BlockQueue<T>::blockEmpty(bool block) {
    if (!block)
        empty_cond.notify_all();
    std::unique_lock<std::mutex> lock(lock_change_mutex);
    block_empty = block;
}

template<typename T>
void BlockQueue<T>::blockFull(bool block) {
    if (!block)
        full_cond.notify_all();
    std::unique_lock<std::mutex> lock(lock_change_mutex);
    block_full = block;
}

template<typename T>
unsigned int BlockQueue<T>::size() const
{
    return q.size();
}

NAMESPACE_END
#endif //BLOCKQUEUE_H
