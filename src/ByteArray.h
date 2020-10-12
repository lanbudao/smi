#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include <string.h>

NAMESPACE_BEGIN

class ByteArrayPrivate;
class ByteArray
{
    DPTR_DECLARE_PRIVATE(ByteArray)
public:
    ByteArray();
    ByteArray(const char* data, size_t size = 0);
    ByteArray& operator = (const ByteArray &other);
    ~ByteArray();

    static ByteArray fromRawData(const char *, size_t size);
    char *data();
    const char *constData() const;
    size_t size() const;
    void reset();
    void fill(char ch, int size = 0);

    void resize(size_t size);

    bool isEmpty() const;

    void setData(const char* data, size_t size);

	void append(const char* data, size_t size);

private:
    DPTR_DECLARE(ByteArray)
};

NAMESPACE_END
#endif //BYTEARRAY_H
