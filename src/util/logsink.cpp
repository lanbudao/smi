#include "logsink.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NAMESPACE_BEGIN

/// Wrapper of timestamp.
class Timestamp
{
public:
    Timestamp() : timestamp_(0) {}
    Timestamp(uint64_t timestamp) : timestamp_(timestamp) {}

    /// Timestamp of current timestamp.
    static Timestamp now() {
        uint64_t timestamp = 0;

#ifdef __linux
        // use gettimeofday(2) is 15% faster then std::chrono in linux.
        struct timeval tv;
        gettimeofday(&tv, NULL);
        timestamp = tv.tv_sec * kUSecPerSec + tv.tv_usec;
#else
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock().now().time_since_epoch())
            .count();
#endif

        return Timestamp(timestamp);
    }

    /// Formatted string of today.
    /// e.g. 20180805
    std::string date() const { return std::string(datetime(), 0, 8); }

    /// Formatted string of current second include date and time.
    /// e.g. 20180805 14:45:20
    std::string datetime() const {
        // reduce count of calling strftime by thread_local.
        static thread_local time_t t_second = 0;
        static thread_local char t_datetime[24]; // 20190816 15:32:25
        time_t nowsec = timestamp_ / kUSecPerSec;
        if (t_second < nowsec) {
            t_second = nowsec;
            struct tm *st_time = localtime(&t_second);
            strftime(t_datetime, sizeof(t_datetime), "%Y%m%d %H:%M:%S", st_time);
        }

        return t_datetime;
    }

    /// Formatted string of current second include time only.
    /// e.g. 144836
    std::string time() const {
        std::string time(datetime(), 9, 16);
        time.erase(std::remove(time.begin(), time.end(), ':'), time.end());
        return time;
    }

    /// Formatted string of current timestamp. eg, 20190701 16:28:30.070981
    /// e.g. 20200709 14:48:36.458074
    std::string formatTimestamp() const {
        char format[28];
        uint32_t micro = static_cast<uint32_t>(timestamp_ % kUSecPerSec);
        snprintf(format, sizeof(format), "%s.%06u", datetime().c_str(), micro);
        return format;
    }

    /// Current timestamp (us).
    /// e.g. 1594277460153980
    uint64_t timestamp() const { return timestamp_; }

private:
    uint64_t timestamp_;
    static const uint32_t kUSecPerSec = 1000000;
};

/// Circle FIFO blocking produce/consume byte queue. Hold log info to wait for
/// background thread consume. It exists in each thread.
class BlockingBuffer {
public:
    BlockingBuffer()
        : producePos_(0), consumePos_(0), consumablePos_(0), produceCount_(0) {}

    /// Get position offset calculated from buffer start.
    uint32_t offsetOfPos(uint32_t pos) const { return pos & (size() - 1); }

    /// Buffer size.
    uint32_t size() const { return kBlockingBufferSize; }

    /// Already used bytes.
    /// It may be called by different threads, so add memory barrier to
    /// ensure the lasted *Pos_ is read.
    uint32_t used() const {
        std::atomic_thread_fence(std::memory_order_acquire);
        return producePos_ - consumePos_;
    }

    /// Unused bytes.
    uint32_t unused() const { return kBlockingBufferSize - used(); }

    /// Reset buffer's position.
    void reset() { producePos_ = consumePos_ = 0; }

    /// The position at the end of the last complete log.
    uint32_t consumable() const {
        std::atomic_thread_fence(std::memory_order_acquire);
        return consumablePos_ - consumePos_;
    }

    /// Increase consumable position with a complete log length \p n .
    void incConsumablePos(uint32_t n) {
        consumablePos_ += n;
        std::atomic_thread_fence(std::memory_order_release);
    }

    /// Peek the produce position in buffer.
    char *peek() { return &storage_[producePos_]; }

    /// consume n bytes data and only move the consume position.
    void consume(uint32_t n) { consumePos_ += n; }

    /// consume \p n bytes data to \p to .
    uint32_t consume(char *to, uint32_t n) {
        // available bytes to consume.
        uint32_t avail = std::min(consumable(), n);

        // offset of consumePos to buffer end.
        uint32_t off2End = std::min(avail, size() - offsetOfPos(consumePos_));

        // first put the data starting from consumePos until the end of buffer.
        memcpy(to, storage_ + offsetOfPos(consumePos_), off2End);

        // then put the rest at beginning of the buffer.
        memcpy(to + off2End, storage_, avail - off2End);

        consumePos_ += avail;
        std::atomic_thread_fence(std::memory_order_release);

        return avail;
    }

    /// Copy \p n bytes log info from \p from to buffer. It will be blocking
    /// when buffer space is insufficient.
    void produce(const char *from, uint32_t n) {
        while (unused() < n)
            /* blocking */;

        // offset of producePos to buffer end.
        uint32_t off2End = std::min(n, size() - offsetOfPos(producePos_));

        // first put the data starting from producePos until the end of buffer.
        memcpy(storage_ + offsetOfPos(producePos_), from, off2End);

        // then put the rest at beginning of the buffer.
        memcpy(storage_, from + off2End, n - off2End);

        produceCount_++;
        producePos_ += n;
        std::atomic_thread_fence(std::memory_order_release);
    }

private:
    static const uint32_t kBlockingBufferSize = 1 << 20; // 1 MB
    uint32_t producePos_;
    uint32_t consumePos_;
    uint32_t consumablePos_; // increase every time with a complete log length.
    uint32_t produceCount_;
    char storage_[kBlockingBufferSize]; // buffer size power of 2.
};

class LogSinkPrivate
{
public:
    LogSinkPrivate(uint32_t rollSize):
        fileCount_(0),
        rollSize_(rollSize),
        writtenBytes_(0),
        fileName_(kDefaultLogFile),
        date_(Timestamp::now().date()),
        fp_(nullptr),

        threadSync_(false),
        threadExit_(false),
        outputFull_(false),
        sinkCount_(0),
        logCount_(0),
        totalSinkTimes_(0),
        totalConsumeBytes_(0),
        perConsumeBytes_(0),
        bufferSize_(1 << 24),
        outputBuffer_(nullptr),
        doubleBuffer_(nullptr),
        threadBuffers_(),
        sinkThread_(),
        bufferMutex_(),
        condMutex_(),
        proceedCond_(),
        hitEmptyCond_()
    {
        outputBuffer_ = static_cast<char *>(malloc(bufferSize_));
        //doubleBuffer_ = static_cast<char *>(malloc(bufferSize_));
        // alloc memory exception handle.
        sinkThread_ = std::thread(&LogSinkPrivate::sinkThreadFunc, this);
    }
    ~LogSinkPrivate();

    size_t write(const char *data, size_t len);

    /// Roll File with size. The new filename contain log file count.
    void rollFile();

    /// Consume the log info produced by the front end to the file.
    void sinkThreadFunc();

    BlockingBuffer *blockingBuffer();

    /// Sink log data \p data of length \p len to file.
    size_t sink(const char *data, size_t len);
    /// List log related statistic.
    void listStatistic() const;

    uint32_t fileCount_;    // file roll count.
    uint32_t rollSize_;     // size of MB.
    uint64_t writtenBytes_; // total written bytes.
    std::string fileName_;
    std::string date_;
    FILE *fp_;
    static const uint32_t kBytesPerMb = 1 << 20;
    static constexpr const char *kDefaultLogFile = "ffproc";

public:
    //---------------------------------->
    bool threadSync_; // front-back-end sync.
    bool threadExit_; // background thread exit flag.
    bool outputFull_; // output buffer full flag.
    uint32_t sinkCount_;             // count of sinking to file.
    std::atomic<uint64_t> logCount_; // count of produced logs.
    uint64_t totalSinkTimes_;        // total time takes of sinking to file.
    uint64_t totalConsumeBytes_;     // total consume bytes.
    uint32_t perConsumeBytes_;       // bytes of consume first-end data per loop.
    uint32_t bufferSize_;            // two buffer size.
    char *outputBuffer_;             // first internal buffer.
    char *doubleBuffer_;             // second internal buffer.
    std::vector<BlockingBuffer *> threadBuffers_;
    std::thread sinkThread_;
    std::mutex bufferMutex_; // internel buffer mutex.
    std::mutex condMutex_;
    std::condition_variable proceedCond_;  // for background thread to proceed.
    std::condition_variable hitEmptyCond_; // for no data to consume.
};

LogSinkPrivate::~LogSinkPrivate()
{
    {
        // notify background thread befor the object detoryed.
        // FIXME: condMutex_ is lock dead sometimes
        while (perConsumeBytes_ != 0) {
            std::unique_lock<std::mutex> lock(condMutex_);
            threadSync_ = true;
            proceedCond_.notify_all();
            //hitEmptyCond_.wait(lock);
            hitEmptyCond_.wait_for(lock, std::chrono::microseconds(1));
        }
    }

    {
        // stop sink thread.
        std::lock_guard<std::mutex> lock(condMutex_);
        threadExit_ = true;
        proceedCond_.notify_all();
    }

    if (sinkThread_.joinable())
        sinkThread_.join();

    free(outputBuffer_);
    free(doubleBuffer_);

    for (auto &buf : threadBuffers_)
        free(buf);

    listStatistic();
    if (fp_) {
        fclose(fp_);
    }
}

size_t LogSinkPrivate::write(const char * data, size_t len)
{
    size_t n = fwrite(data, 1, len, fp_);
    size_t remain = len - n;
    while (remain > 0) {
        size_t x = fwrite(data + n, 1, remain, fp_);
        if (x == 0) {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "File write error: %s\n", strerror(err));
            break;
        }
        n += x;
        remain -= x;
    }

    fflush(fp_);
    writtenBytes_ += len - remain;

    return len - remain;
}

void LogSinkPrivate::rollFile()
{
    if (fp_)
        fclose(fp_);

    std::string file(fileName_ + '-' + date_);
    if (fileCount_)
        file += '(' + std::to_string(fileCount_) + ").log";
    else
        file += ".log";

    fp_ = fopen(file.c_str(), "a+");
    if (!fp_) {
        fprintf(stderr, "Faild to open file:%s\n", file.c_str());
        exit(-1);
    }

    fileCount_++;
}

void LogSinkPrivate::sinkThreadFunc()
{
    while (!threadExit_) {
        // move front-end data to internal buffer.
        {
            std::lock_guard<std::mutex> lock(bufferMutex_);
            uint32_t bbufferIdx = 0;
            // while (!threadExit_ && !outputFull_ && !threadBuffers_.empty()) {
            while (!threadExit_ && !outputFull_ &&
                (bbufferIdx < threadBuffers_.size())) {
                BlockingBuffer *bbuffer = threadBuffers_[bbufferIdx];
                uint32_t consumableBytes = bbuffer->consumable();

                if (bufferSize_ - perConsumeBytes_ < consumableBytes) {
                    outputFull_ = true;
                    break;
                }

                if (consumableBytes > 0) {
                    uint32_t n = bbuffer->consume(outputBuffer_ + perConsumeBytes_,
                        consumableBytes);
                    perConsumeBytes_ += n;
                }
                else {
                    // threadBuffers_.erase(threadBuffers_.begin() +
                    // bbufferIdx);
                }
                bbufferIdx++;
            }
        }

        // not data to sink, go to sleep, 50us.
        if (perConsumeBytes_ == 0) {
            std::unique_lock<std::mutex> lock(condMutex_);

            // if front-end generated sync operation, consume again.
            if (threadSync_) {
                threadSync_ = false;
                continue;
            }

            hitEmptyCond_.notify_one();
            proceedCond_.wait_for(lock, std::chrono::microseconds(50));
        }
        else {
            uint64_t beginTime = Timestamp::now().timestamp();
            sink(outputBuffer_, perConsumeBytes_);
            uint64_t endTime = Timestamp::now().timestamp();

            totalSinkTimes_ += endTime - beginTime;
            sinkCount_++;
            totalConsumeBytes_ += perConsumeBytes_;
            perConsumeBytes_ = 0;
            outputFull_ = false;
        }
    }
}

/**
 * thread_local is used to new blocking buffer for every thread,
 * such as main-thread, video-thread, audio-thread etc.
 */
BlockingBuffer *LogSinkPrivate::blockingBuffer()
{
    static thread_local BlockingBuffer *buf = nullptr;
    if (!buf) {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        buf = static_cast<BlockingBuffer *>(malloc(sizeof(BlockingBuffer)));
        threadBuffers_.push_back(buf);
    }
    return buf;
}

size_t LogSinkPrivate::sink(const char * data, size_t len)
{
    if (fp_ == nullptr)
        rollFile();

    std::string today(Timestamp::now().date());
    if (date_.compare(today)) {
        date_.assign(today);
        fileCount_ = 0;
        rollFile();
    }

    uint64_t rollBytes = rollSize_ * kBytesPerMb;
    if (writtenBytes_ % rollBytes + len > rollBytes)
        rollFile();

    return write(data, len);
}

void LogSinkPrivate::listStatistic() const
{
    printf("\n");
    printf("=== Logging System Related Data ===\n");
    printf("  Total produced logs count: [%" PRIu64 "].\n", logCount_.load());
    printf("  Total bytes of sinking to file: [%" PRIu64 "] bytes.\n",
        totalConsumeBytes_);
    printf("  Average bytes of sinking to file: [%" PRIu64 "] bytes.\n",
        sinkCount_ == 0 ? 0 : totalConsumeBytes_ / sinkCount_);
    printf("  Count of sinking to file: [%u].\n", sinkCount_);
    printf("  Total microseconds takes of sinking to file: [%" PRIu64 "] us.\n",
        totalSinkTimes_);
    printf("  Average microseconds takes of sinking to file: [%" PRIu64 "] us.\n",
        sinkCount_ == 0 ? 0 : totalSinkTimes_ / sinkCount_);
    printf("\n");
}

LogSink::LogSink(uint32_t rollSize):
    d_ptr(new LogSinkPrivate(rollSize))
{

}

LogSink::~LogSink()
{
}

void LogSink::setLogFile(const char * file)
{
    DPTR_D(LogSink);
    d->fileName_.assign(file);
    d->rollFile();
}

void LogSink::setRollSize(uint32_t size)
{
    DPTR_D(LogSink);
    d->rollSize_ = size;
}

void LogSink::produce(const char * data, size_t n)
{
    DPTR_D(LogSink);
    d->blockingBuffer()->produce(data, static_cast<uint32_t>(n));
}

void LogSink::incConsumable(uint32_t n)
{
    DPTR_D(LogSink);
    d->blockingBuffer()->incConsumablePos(n);
    d->logCount_.fetch_add(1, std::memory_order_relaxed);
}


NAMESPACE_END