#include "Subtitle.h"
#include "SubtitleDecoder.h"
#include "AVLog.h"
#include "CThread.h"
#include "util/mkid.h"

NAMESPACE_BEGIN

class LoadAsync: public CThread {
public:
    LoadAsync(SubtitlePrivate *d): priv(d) {}
    void run();
private:
    SubtitlePrivate *priv;
};

SubtitleDecoderId SubtitleDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;

class SubtitlePrivate
{
public:
    SubtitlePrivate():
        enabled(false),
        time_stamp(0),
        decoder(nullptr),
        load_async(nullptr)
    {
        decoder = SubtitleDecoder::create(SubtitleDecoderId_FFmpeg);
        if (!decoder) {
            AVWarning("Subtitle decoder inited failed!\n");
        }
    }
    virtual ~SubtitlePrivate()
    {
        if (load_async) {
            delete load_async;
            load_async = nullptr;
        }
        if (decoder) {
            delete decoder;
            decoder = nullptr;
        }
    }

    void reset()
    {

    }

    void loadInternal();

    bool enabled;
    std::string fileName;
    std::string codec;
    double time_stamp;
    SubtitleDecoder *decoder;
    LoadAsync *load_async;
};

void SubtitlePrivate::loadInternal()
{

}

void LoadAsync::run()
{
    priv->loadInternal();
}

Subtitle::Subtitle():
    d_ptr(new SubtitlePrivate)
{

}

Subtitle::~Subtitle()
{

}

bool Subtitle::enabled() const
{
    DPTR_D(const Subtitle);
    return d->enabled;
}

void Subtitle::setEnabled(bool b)
{
    DPTR_D(Subtitle);
    d->enabled = b;
}

void Subtitle::setFile(const std::string &name)
{
    DPTR_D(Subtitle);
    d->fileName = name;
}

void Subtitle::setCodec(const std::string &codec)
{
    DPTR_D(Subtitle);
    d->codec = codec;
}

void Subtitle::load()
{
    DPTR_D(Subtitle);
    if (d->load_async) {
        d->load_async = new LoadAsync(d);
    }
    d->load_async->run();
}

bool Subtitle::processHeader(MediaInfo *info)
{
    DPTR_D(Subtitle);
    return d->decoder->processHeader(info);
}

bool Subtitle::processLine(Packet *pkt)
{
    DPTR_D(Subtitle);
    SubtitleFrame frame = d->decoder->processLine(pkt);
    return true;
}

void Subtitle::setTimestamp(double t)
{
    DPTR_D(Subtitle);
    d->time_stamp = t;
}

SubtitleFrame Subtitle::frame()
{
    return SubtitleFrame();
}

NAMESPACE_END
