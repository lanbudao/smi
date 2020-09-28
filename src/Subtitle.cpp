#include "Subtitle.h"
#include "SubtitleDecoder.h"
#include "AVLog.h"
#include "CThread.h"

NAMESPACE_BEGIN

class LoadAsync: public CThread {
public:
    LoadAsync(SubtitlePrivate *d): priv(d) {}
    void run();
private:
    SubtitlePrivate *priv;
};

class SubtitlePrivate
{
public:
    SubtitlePrivate():
        enabled(false),
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

    void loadInternal()
    {
        FILE *f = fopen(fileName.c_str(), "r");
        if (!f)
            return;
        //TODO

        fclose(f);
    }

    bool enabled;
    std::string fileName;
    std::string codec;
    SubtitleDecoder *decoder;
    LoadAsync *load_async;
};

void LoadAsync::run()
{
    priv->loadInternal();
}

Subtitle::Subtitle()
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

void Subtitle::setFile(const string &fileName)
{
    DPTR_D(Subtitle);
    d->fileName = fileName;
}

void Subtitle::setCodec(const string &codec)
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

bool Subtitle::processLine(const ByteArray &data, double pts, double duration)
{
    DPTR_D(Subtitle);

    return true;
}

NAMESPACE_END
