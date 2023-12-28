#ifndef LOGSINK_H
#define LOGSINK_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "utils/Singleton.h"

#include <stdint.h>

NAMESPACE_BEGIN

#define G_LOGSINK LogSink::instance()
class LogSinkPrivate;
class LogSink: public Singleton<LogSink>
{
    DPTR_DECLARE_PRIVATE(LogSink)
public:
    LogSink(uint32_t rollSize = 10);
    ~LogSink();

    void setLogFile(const char *file);

    void setRollSize(uint32_t size);

    /**
     * @brief Produce log data \p data to BlockingBuffer in each thread.
     */
    void produce(const char *data, size_t n);

    /**
     * @brief Increase BlockingBuffer consumable position.
     */
    void incConsumable(uint32_t n);

private:
    DPTR_DECLARE(LogSink);
};

NAMESPACE_END

#endif // LOGSINK_H