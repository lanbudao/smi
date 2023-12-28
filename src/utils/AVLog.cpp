#include "AVLog.h"

#include <cstdarg>
#include <thread>
#include <vector>
#include <string.h>
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

#include "utils/logsink.h"

NAMESPACE_BEGIN

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

static const char* log_level_string[LogFatal] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

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

static bool g_output_log = false;
static LogLevel g_log_level = LogWarning;
Debug::Debug(LogLevel level):
    d_ptr(new DebugPrivate)
{
    DPTR_D(Debug);
    d->level = level;
#ifndef HIDE_LOG_DETAIL
    *this << Timestamp::now().formatTimestamp() << ' '
        << gettid() << ' '
        << log+log_level_string[d->level] << "  ";
#endif
}

Debug::~Debug()
{
    DPTR_D(Debug);
    if (d->context.file && d->context.function)
        *this << "  " << d->context.file << ':' << d->context.function << " " << d->context.line;
    /// Move consumable position with increaing to ensure a complete log is consumed each time.
    if (g_output_log && d->level >= g_log_level) {
        G_LOGSINK.produce(d->text.c_str(), d->count);
        G_LOGSINK.incConsumable(d->count); // already produce a complete log.
    }
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

void setLogOut(bool b)
{
    g_output_log = b;
}

void setLogLevel(LogLevel level)
{
    g_log_level = level;
}

LogLevel getLogLevel()
{
    return g_log_level;
}

void setLogFile(const char *file)
{
    if (!g_output_log)
        return;
    G_LOGSINK.setLogFile(file);
}

void setRollSize(uint32_t MB)
{
    if (!g_output_log)
        return;
    G_LOGSINK.setRollSize(MB);
}

NAMESPACE_END
