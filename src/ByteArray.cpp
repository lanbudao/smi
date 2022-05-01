#include "ByteArray.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/fifo.h"
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN

class ByteArrayPrivate
{
public:
    ByteArrayPrivate(int s = 0):
        data(nullptr)
    {
        size = s;
        if (size == 0)
            return;
        data = av_fifo_alloc(s);
        av_fifo_reset(data);
    }
    ~ByteArrayPrivate()
    {
        if (data)
            av_fifo_freep(&data);
    }
    AVFifoBuffer *data;
    size_t size;
};

ByteArray::ByteArray():
    d_ptr(new ByteArrayPrivate())
{

}

ByteArray::ByteArray(const char *data, size_t size):
    d_ptr(new ByteArrayPrivate(size))
{
    DPTR_D(ByteArray);
    if (data && size == 0) {
        size = sizeof(data);
    }
    if (size > 0) {
        setData(data, size);
    }
}

ByteArray::~ByteArray()
{

}

ByteArray ByteArray::fromRawData(const char *p, size_t size)
{
    ByteArray data(p, size);
    return data;
}

char *ByteArray::data()
{
    DPTR_D(ByteArray);
    return reinterpret_cast<char *>(d->data->buffer);
}

const char *ByteArray::constData() const
{
    DPTR_D(const ByteArray);
    if (!d->data)
        return nullptr;
    return reinterpret_cast<const char *>(d->data->buffer);
}

size_t ByteArray::size() const
{
    DPTR_D(const ByteArray);
    return d->size;
}

void ByteArray::reset()
{
    DPTR_D(ByteArray);
    if (d->data)
        av_fifo_reset(d->data);
}

void ByteArray::fill(char ch, int size)
{
    DPTR_D(ByteArray);
    resize(size < 0 ? d->size : size);
    if (d->size)
        memset(d->data->buffer, ch, d->size);
}

void ByteArray::resize(size_t size)
{
    DPTR_D(ByteArray);
    if (size == 0 || size == d->size) {
        return;
    }
    if (!d->data) {
        d->data = av_fifo_alloc(size);
        av_fifo_reset(d->data);
    } else {
        av_fifo_realloc2(d->data, size);
    }
    d->size = size;
}

bool ByteArray::isEmpty() const
{
    DPTR_D(const ByteArray);
    return !d->data || d->size == 0;
}

ByteArray &ByteArray::operator =(const ByteArray &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

void ByteArray::setData(const char *data, size_t size)
{
    DPTR_D(ByteArray);
    if (size > 0) {
        if (size != d->size) {
            if (!d->data) {
                d->data = av_fifo_alloc(size);
                av_fifo_reset(d->data);
            } else {
                av_fifo_realloc2(d->data, size);
            }
        }
        d->size = size;
        av_fifo_generic_write(d->data, (void*)data, size, nullptr);
    }
}

void ByteArray::append(const char* data, size_t size)
{
	DPTR_D(ByteArray);
	av_fifo_grow(d->data, size);
	av_fifo_generic_write(d->data, (void*)data, size, nullptr);
	d->size = av_fifo_size(d->data);
}

void ByteArray::clear()
{
    DPTR_D(ByteArray);

    if (d->data)
        av_fifo_freep(&d->data);
}

NAMESPACE_END
