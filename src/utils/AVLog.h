#ifndef AVLOG_H
#define AVLOG_H

#include "sdk/DPTR.h"
#include "sdk/global.h"

NAMESPACE_BEGIN

#ifdef MESSAGELOGCONTEXT
  #define MESSAGELOG_FILE __FILE__
  #define MESSAGELOG_LINE __LINE__
  #define MESSAGELOG_FUNC __FUNCSIG__
#else
  #define MESSAGELOG_FILE nullptr
  #define MESSAGELOG_LINE 0
  #define MESSAGELOG_FUNC nullptr
#endif

/**
 * @brief Do not show Timestamp, thread id and log level defaultly
 */
#define HIDE_LOG_DETAIL

#define AVDebug      MessageLogger(MESSAGELOG_FILE, MESSAGELOG_LINE, MESSAGELOG_FUNC).debug
#define AVInfo       MessageLogger(MESSAGELOG_FILE, MESSAGELOG_LINE, MESSAGELOG_FUNC).info
#define AVWarning    MessageLogger(MESSAGELOG_FILE, MESSAGELOG_LINE, MESSAGELOG_FUNC).warning
#define AVError      MessageLogger(MESSAGELOG_FILE, MESSAGELOG_LINE, MESSAGELOG_FUNC).error
#define AVFatal      MessageLogger(MESSAGELOG_FILE, MESSAGELOG_LINE, MESSAGELOG_FUNC).fatal

/**
 * @brief If you need to print the output to the local file, you need to
 * call 'setLogFile' to specify the file to print.
 */

class SMI_EXPORT MessageLogContext
{
public:
    MessageLogContext()
        : version(2), line(0), file(nullptr), function(nullptr), category(nullptr) {}
    MessageLogContext(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
        : version(2), line(lineNumber), file(fileName), function(functionName), category(categoryName) {}

    int version;
    int line;
    const char *file;
    const char *function;
    const char *category;

private:
    friend class MessageLogger;
    friend class Debug;
};

class DebugPrivate;
class SMI_EXPORT Debug
{
    DPTR_DECLARE_PRIVATE(Debug)
public:
    Debug(LogLevel level = LogWarning);
    ~Debug();

    Debug log(const char *fmt, va_list list);
    Debug &operator<<(bool arg);
    Debug &operator<<(char arg);
    Debug &operator<<(int16_t arg);
    Debug &operator<<(uint16_t arg);
    Debug &operator<<(int32_t arg);
    Debug &operator<<(uint32_t arg);
    Debug &operator<<(int64_t arg);
    Debug &operator<<(uint64_t arg);
    Debug &operator<<(double arg);
    Debug &operator<<(const char *arg);
    Debug &operator<<(const std::string &arg);

    void setContext(MessageLogContext &ctx);

private:
    class SMI_EXPORT DPTR_DECLARE(Debug)
};

class SMI_EXPORT MessageLogger
{
    DISABLE_COPY(MessageLogger)
public:
    MessageLogger();
    MessageLogger(const char *file, int line, const char *function);

    void debug(const char *msg, ...);
    void info(const char *msg, ...);
    void warning(const char *msg, ...);
    void error(const char *msg, ...);
    void fatal(const char *msg, ...);

    Debug debug() const;
    Debug info() const;
    Debug warning() const;
    Debug error() const;
    Debug fatal() const;

private:
    class SMI_EXPORT MessageLogContext context;
};

/**
 * @brief Whether to output log, default value is false
 */
void SMI_EXPORT setLogOut(bool b);
/**
 * @brief Set the level of log, default value is LogWarning
 */
void SMI_EXPORT setLogLevel(LogLevel level);

/**
 * @brief Get log level.
 */
LogLevel SMI_EXPORT getLogLevel();

/**
 * @brief Set log file (basename and dir, no suffix), defalut `./run`
 * The Program will add suffix (`.log`) automatically
 */ 
/// 
void SMI_EXPORT setLogFile(const char *file);

void SMI_EXPORT setRollSize(uint32_t MB);

NAMESPACE_END

#endif //AVLOG_H
