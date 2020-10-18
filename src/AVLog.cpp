#include "AVLog.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <inttypes.h>
#include <stdint.h> // uint32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdarg>

#ifdef __linux
#include <sys/time.h>
#include <sys/syscall.h> // gettid().
#include <unistd.h>
typedef pid_t thread_id_t;
#else
#include <chrono>
#include <sstream>
typedef unsigned int thread_id_t; // MSVC
#endif

NAMESPACE_BEGIN

/// Wrapper of timestamp.
class Timestamp {
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

// The digits table is used to look up for number within 100.
// Each two character corresponds to one digit and ten digits.
static constexpr char DigitsTable[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0',
    '7', '0', '8', '0', '9', '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', '2', '0', '2', '1', '2',
    '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3',
    '7', '3', '8', '3', '9', '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', '5', '0', '5', '1', '5',
    '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6',
    '7', '6', '8', '6', '9', '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', '8', '0', '8', '1', '8',
    '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9',
    '7', '9', '8', '9', '9'};

static size_t u2a(uint64_t number, char *to) {
    char buf[24];
    char *p = buf;
    size_t length = 0;

    while (number >= 100) {
        const unsigned idx = (number % 100) << 1;
        number /= 100;
        *p++ = DigitsTable[idx + 1];
        *p++ = DigitsTable[idx];
    }

    if (number < 10) {
        *p++ = number + '0';
    } else {
        const unsigned idx = number << 1;
        *p++ = DigitsTable[idx + 1];
        *p++ = DigitsTable[idx];
    }

    length = p - buf;

    do {
        *to++ = *--p;
    } while (p != buf);
    *to = '\0';

    return length;
}

static size_t i2a(int64_t number, char *to) {
    uint64_t n = static_cast<uint64_t>(number);
    size_t sign = 0;
    if (number < 0) {
        *to++ = '-';
        n = ~n + 1;
        sign = 1;
    }

    return u2a(n, to) + sign;
}

/// Convert unsigned integer \p n to string \p to , and return string length.
static size_t u16toa(uint16_t n, char *to) { return u2a(n, to); }
static size_t u32toa(uint32_t n, char *to) { return u2a(n, to); }
static size_t u64toa(uint64_t n, char *to) { return u2a(n, to); }

/// Convert signed integer \p n to string \p to , and return string length.
static size_t i16toa(int16_t n, char *to) { return i2a(n, to); }
static size_t i32toa(int32_t n, char *to) { return i2a(n, to); }
static size_t i64toa(int64_t n, char *to) { return i2a(n, to); }

static inline thread_id_t gettid() {
    static thread_local thread_id_t t_tid = 0;

    if (t_tid == 0) {
#ifdef __linux
        t_tid = syscall(__NR_gettid);
#else
        std::stringstream ss;
        ss << std::this_thread::get_id();
        ss >> t_tid;
#endif
    }
    return t_tid;
}

// Stringify log level with width of 5.
static const char *stringifyLogLevel(LogLevel level) {
    switch (level) {
    case LogFatal:
        return "FATAL";
    case LogError:
        return "ERROR";
    case LogWarning:
        return "WARN ";
    case LogInfo:
        return "INFO ";
    case LogDebug:
        return "DEBUG";
    case LogTrace:
        return "TRACE";
    }
    return "NONE";
}

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

/// Sink log info on file, and the file automatic roll with a setting size.
/// If set log file, initially the log file format is `filename.date.log `.
class LogSink {
public:
    LogSink() : LogSink(10) {}

    LogSink(uint32_t rollSize)
        : level_(LogWarning),
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
        doubleBuffer_ = static_cast<char *>(malloc(bufferSize_));
        // alloc memory exception handle.
        sinkThread_ = std::thread(&LogSink::sinkThreadFunc, this);

    }
    ~LogSink()
    {
        {
            // notify background thread befor the object detoryed.
            std::unique_lock<std::mutex> lock(condMutex_);
            threadSync_ = true;
            proceedCond_.notify_all();
            //hitEmptyCond_.wait(lock);
            hitEmptyCond_.wait_for(lock, std::chrono::microseconds(50));
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

    static LogSink* instance()
    {
        static LogSink sink;
        return &sink;
    }

    void setLogFile(const char *file) {
        fileName_.assign(file);
        rollFile();
    }

    void setRollSize(uint32_t size) { rollSize_ = size; }

    LogLevel getLogLevel() const { return level_; };
    void setLogLevel(LogLevel level) { level_ = level; }
    /// Roll File with size. The new filename contain log file count.
    void rollFile() {
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

    /// Sink log data \p data of length \p len to file.
    size_t sink(const char *data, size_t len) {
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

    /// Produce log data \p data to BlockingBuffer in each thread.
    void produce(const char *data, size_t n) {
        blockingBuffer()->produce(data, static_cast<uint32_t>(n));
    }

    /// Increase BlockingBuffer consumable position.
    void incConsumable(uint32_t n) {
        blockingBuffer()->incConsumablePos(n);
        logCount_.fetch_add(1, std::memory_order_relaxed);
    }

    /// List log related statistic.
    void listStatistic() const {
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


private:
    LogLevel level_;
    uint32_t fileCount_;    // file roll count.
    uint32_t rollSize_;     // size of MB.
    uint64_t writtenBytes_; // total written bytes.
    std::string fileName_;
    std::string date_;
    FILE *fp_;
    static const uint32_t kBytesPerMb = 1 << 20;
    static constexpr const char *kDefaultLogFile = "ffpro";

    size_t write(const char *data, size_t len) {
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

    /// Consume the log info produced by the front end to the file.
    void sinkThreadFunc();

    BlockingBuffer *blockingBuffer();

    //---------------------------------->
    bool threadSync_; // front-back-end sync.
    bool threadExit_; // background thread exit flag.
    bool outputFull_; // output buffer full flag.
//    LogLevel level_;
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

void LogSink::sinkThreadFunc()
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
                } else {
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
        } else {
            uint64_t beginTime = Timestamp::now().timestamp();
            LogSink::instance()->sink(outputBuffer_, perConsumeBytes_);
            uint64_t endTime = Timestamp::now().timestamp();

            totalSinkTimes_ += endTime - beginTime;
            sinkCount_++;
            totalConsumeBytes_ += perConsumeBytes_;
            perConsumeBytes_ = 0;
            outputFull_ = false;
        }
    }
}

BlockingBuffer *LogSink::blockingBuffer()
{
    static thread_local BlockingBuffer *buf = nullptr;
    if (!buf) {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        buf = static_cast<BlockingBuffer *>(malloc(sizeof(BlockingBuffer)));
        threadBuffers_.push_back(buf);
    }
    return buf;
}


class DebugPrivate
{
public:
    DebugPrivate():
        level(LogWarning),
        count(0)
    {
    }
    ~DebugPrivate()
    {
        switch (level) {
        case LogTrace:
        case LogDebug:
        case LogInfo:
        case LogWarning:
            fprintf(stdout, "%s", text.c_str());
            fflush(stdout);
            break;
        case LogError:
        case LogFatal:
            fprintf(stderr, "%s", text.c_str());
            fflush(stderr);
            break;
        }
    }

    void append(const char *data, size_t n)
    {
        text.append(data);
        count += static_cast<uint32_t>(n);
    }

    void append(const char *data) { append(data, strlen(data)); }

    LogLevel level;
    uint32_t count;
    MessageLogContext context;
    std::string text;
};

Debug::Debug(LogLevel level):
    d_ptr(new DebugPrivate)
{
    DPTR_D(Debug);
    d->level = level;
#ifndef HIDE_LOG_DETAIL
    *this << Timestamp::now().formatTimestamp() << ' '
        << gettid() << ' '
        << stringifyLogLevel(d->level) << "  ";
#endif
}

Debug::~Debug()
{
    DPTR_D(Debug);
    if (d->context.file && d->context.function)
        *this << "  " << d->context.file << ':' << d->context.function << " " << d->context.line;
    /// Move consumable position with increaing to ensure a complete log is consumed each time.
    LogSink::instance()->produce(d->text.c_str(), d->count);
    LogSink::instance()->incConsumable(d->count); // already produce a complete log.
}

Debug Debug::log(const char *fmt, va_list marker)
{
    DPTR_D(Debug);
    std::string output;
    int length = _vscprintf(fmt, marker) + 1;
    std::vector<char> buffer(length, '\0');
    int ret = vsnprintf(&buffer[0], (size_t)length, fmt, marker);
    if (ret > 0)
        output = &buffer[0];
    d->append(output.c_str(), output.length());
    return *this;
}

Debug& Debug::operator<<(bool arg)
{
    DPTR_D(Debug);
       if (arg)
           d->append("true", 4);
       else
           d->append("false", 5);
       return *this;
}

void Debug::setContext(MessageLogContext &ctx)
{
    DPTR_D(Debug);
    d->context = ctx;
}

Debug& Debug::operator<<(char arg)
{
    DPTR_D(Debug);
       d->append(&arg, 1);
       return *this;
   }

Debug& Debug::operator<<(int16_t arg)
{
    DPTR_D(Debug);
       char tmp[8];
       size_t len = i16toa(arg, tmp);
       d->append(tmp, len);
       return *this;
   }


Debug& Debug::operator<<(uint16_t arg)
{
    DPTR_D(Debug);
    char tmp[8];
    size_t len = u16toa(arg, tmp);
    d->append(tmp, len);
    return *this;
}

Debug& Debug::operator<<(int32_t arg)
{
    DPTR_D(Debug);
    char tmp[12];
    size_t len = i32toa(arg, tmp);
    d->append(tmp, len);
    return *this;
}

Debug& Debug::operator<<(uint32_t arg)
{
    DPTR_D(Debug);
    char tmp[12];
    size_t len = u32toa(arg, tmp);
    d->append(tmp, len);
    return *this;
}

Debug& Debug::operator<<(int64_t arg)
{
    DPTR_D(Debug);
    char tmp[24];
    size_t len = i64toa(arg, tmp);
    d->append(tmp, len);
    return *this;
}

Debug& Debug::operator<<(uint64_t arg)
{
    DPTR_D(Debug);
    char tmp[24];
    size_t len = u64toa(arg, tmp);
    d->append(tmp, len);
    return *this;
}

Debug& Debug::operator<<(double arg)
{
    DPTR_D(Debug);
    d->append(std::to_string(arg).c_str());
    return *this;
}

Debug& Debug::operator<<(const char *arg)
{
    DPTR_D(Debug);
    d->append(arg);
    return *this;
}

Debug& Debug::operator<<(const std::string &arg)
{
    DPTR_D(Debug);
    d->append(arg.c_str(), arg.length());
    return *this;
}

MessageLogger::MessageLogger()
{

}

MessageLogger::MessageLogger(const char *file, int line, const char *function)
    : context(file, line, function, "default")
{

}

void MessageLogger::debug(const char *msg, ...)
{
    Debug dbg = Debug(LogDebug);
    dbg.setContext(context);
    va_list ap;
    va_start(ap, msg);
    dbg.log(msg, ap);
    va_end(ap);
}

void MessageLogger::info(const char *msg, ...)
{
    Debug dbg = Debug(LogInfo);
    dbg.setContext(context);
    va_list ap;
    va_start(ap, msg);
    dbg.log(msg, ap);
    va_end(ap);
}

void MessageLogger::warning(const char *msg, ...)
{
    Debug dbg = Debug(LogWarning);
    dbg.setContext(context);
    va_list ap;
    va_start(ap, msg);
    dbg.log(msg, ap);
    va_end(ap);
}

void MessageLogger::error(const char *msg, ...)
{
    Debug dbg = Debug(LogError);
    dbg.setContext(context);
    va_list ap;
    va_start(ap, msg);
    dbg.log(msg, ap);
    va_end(ap);
}

void MessageLogger::fatal(const char *msg, ...)
{
    Debug dbg = Debug(LogFatal);
    dbg.setContext(context);
    va_list ap;
    va_start(ap, msg);
    dbg.log(msg, ap);
    va_end(ap);
}

Debug MessageLogger::debug() const
{
    Debug dbg = Debug(LogDebug);
    return dbg;
}

Debug MessageLogger::info() const
{
    Debug dbg = Debug(LogInfo);
    return dbg;
}

Debug MessageLogger::warning() const
{
    Debug dbg = Debug(LogWarning);
    return dbg;
}

Debug MessageLogger::error() const
{
    Debug dbg = Debug(LogError);
    return dbg;
}

Debug MessageLogger::fatal() const
{
    Debug dbg = Debug(LogFatal);
    return dbg;
}

void setLogLevel(LogLevel level)
{
    LogSink::instance()->setLogLevel(level);
}

LogLevel getLogLevel()
{
    return LogSink::instance()->getLogLevel();
}

void setLogFile(const char *file)
{
    LogSink::instance()->setLogFile(file);
}

void setRollSize(uint32_t MB)
{
    LogSink::instance()->setRollSize(MB);
}

//void MessageLogContext::copy(const MessageLogContext &ctx)
//{
//    version = ctx.version;
//    line = ctx.line;
//    file = ctx.file;
//    function = ctx.function;
//    category = ctx.category;
//}

NAMESPACE_END
