#include "mediaio.h"
#include "mediaio_p.h"
#include "AVLog.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(MediaIO)

typedef int MediaIOId;

MediaIO::MediaIO(MediaIOPrivate *d):
    d_ptr(d)
{

}

MediaIO::~MediaIO()
{
    release();
}

std::list<std::string> MediaIO::supportProtocols()
{
    static std::list<std::string> names;
    if (!names.empty())
        return names;
    std::vector<const char*> ns(MediaIOFactory::instance().registeredNames());
    std::vector<const char*>::iterator it = ns.begin();
    for (; it != ns.end(); ++it) {
        names.push_back(*it);
    }
    return names;
}

MediaIO* MediaIO::createForProtocol(const std::string &protocol)
{
    std::vector<MediaIOId> ids(MediaIOFactory::instance().registeredIds());
    std::vector<MediaIOId>::iterator it = ids.begin();
    for (; it != ids.end(); ++it) {
        if (MediaIOFactory::instance().name(*it) == protocol) {
            MediaIO *in = MediaIO::create(*it);
            return in;
        }
    }
    return 0;
}

MediaIO* MediaIO::createForUrl(const std::string &url)
{
    const int p = url.find(":");
    if (p < 0)
        return 0;
    MediaIO *io = MediaIO::createForProtocol(url.substr(0, p));
    if (!io)
        return 0;
    io->setUrl(url);
    return io;
}

static int av_read(void *opaque, unsigned char *buf, int buf_size)
{
    MediaIO* io = static_cast<MediaIO*>(opaque);
    return io->read(buf, buf_size);
}

static int av_write(void *opaque, unsigned char *buf, int buf_size)
{
    MediaIO* io = static_cast<MediaIO*>(opaque);
    return io->write(buf, buf_size);
}

static int64_t av_seek(void *opaque, int64_t offset, int whence)
{
    if (whence == SEEK_SET && offset < 0)
        return -1;
    MediaIO* io = static_cast<MediaIO*>(opaque);
    if (!io->isSeekable()) {
        AVWarning("Can not seek. MediaIO is not a seekable IO");
        return -1;
    }
    if (whence == AVSEEK_SIZE) {
        // return the filesize without seeking anywhere. Supporting this is optional.
        return io->size() > 0 ? io->size() : 0;
    }
    if (!io->seek(offset, whence))
        return -1;
    return io->position();
}

void MediaIO::setUrl(const std::string &url)
{
    DPTR_D(MediaIO);
    if (d->url == url)
        return;
    // TODO: check protocol
    d->url = url;
    onUrlChanged();
}

std::string MediaIO::url() const
{
    return d_func()->url;
}

bool MediaIO::setAccessMode(AccessMode value)
{
    DPTR_D(MediaIO);
    if (d->mode == value)
        return true;
    if (value == Write && !isWritable()) {
        AVWarning("Can not set Write access mode to this MediaIO\n");
        return false;
    }
    d->mode = value;
    return true;
}

MediaIO::AccessMode MediaIO::accessMode() const
{
    return d_func()->mode;
}

const std::list<std::string>& MediaIO::protocols() const
{
    static  std::list<std::string> no_protocols;
    return no_protocols;
}

#define IODATA_BUFFER_SIZE 32768 // TODO: user configurable
static const int kBufferSizeDefault = 32768;

void MediaIO::setBufferSize(int value)
{
    DPTR_D(MediaIO);
    if (d->buffer_size == value)
        return;
    d->buffer_size = value;
}

int MediaIO::bufferSize() const
{
    return d_func()->buffer_size;
}

void* MediaIO::avioContext()
{
    DPTR_D(MediaIO);
    if (d->ctx)
        return d->ctx;
    // buffer will be released in av_probe_input_buffer2=>ffio_rewind_with_probe_data. always is? may be another context
    unsigned char* buf = (unsigned char*)av_malloc(IODATA_BUFFER_SIZE);
    // open for write if 1. SET 0 if open for read otherwise data ptr in av_read(data, ...) does not change
    const int write_flag = (accessMode() == Write) && isWritable();
    d->ctx = avio_alloc_context(buf, 
        bufferSize() > 0 ? bufferSize() : kBufferSizeDefault, write_flag,
        this,
        &av_read,
        write_flag ? &av_write : NULL,
        &av_seek);
    // if seekable==false, containers that estimate duration from pts(or bit rate) will not seek to the last frame when computing duration
    // but it's still seekable if call seek outside(e.g. from demuxer)
    // TODO: isVariableSize: size = -real_size
    d->ctx->seekable = isSeekable() && !isVariableSize() ? AVIO_SEEKABLE_NORMAL : 0;
    return d->ctx;
}

void MediaIO::release()
{
    DPTR_D(MediaIO);
    if (!d->ctx)
        return;
    // avio_close is called by avformat_close_input. here we only allocate but no open
    av_freep(&d->ctx->buffer);
    av_freep(&d->ctx);
}

NAMESPACE_END