#include "Subtitle.h"
#include "SubtitleDecoder.h"
#include "AVLog.h"
#include "CThread.h"
#include "util/mkid.h"
extern "C" {
#include "libavcodec/avcodec.h"
}
NAMESPACE_BEGIN

class SubtitleFramePrivate
{
public:
    SubtitleFramePrivate()
    {

    }
    ~SubtitleFramePrivate()
    {
        clear();
    }

    void clear()
    {
        texts.clear();
        std::vector<AVFrame*>::iterator it = images.begin();
        for (; it != images.end(); ++it) {
            av_frame_free(&(*it));
        }
    }

    std::vector<std::string> texts; //used for SUBTITLE_ASS and SUBTITLE_TEXT
    std::vector<AVFrame*> images;   //used for SUBTITLE_PIXMAP
};

SubtitleFrame::SubtitleFrame():
    d_ptr(new SubtitleFramePrivate),
    start(0),
    stop(0),
    type(SubtitleText)
{

}

SubtitleFrame::~SubtitleFrame()
{

}

void SubtitleFrame::push_back(const std::string & text)
{
    d_func()->texts.push_back(text);
}

const std::vector<std::string>& SubtitleFrame::texts() const
{
    return d_func()->texts;
}

const std::vector<AVFrame*>& SubtitleFrame::images() const
{
    return d_func()->images;
}

void SubtitleFrame::reset()
{
    start = stop = 0;
    d_func()->clear();
}

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

    bool prepareFrame();

    bool enabled;
    std::string fileName;
    std::string codec;
    double time_stamp;
    SubtitleDecoder *decoder;
    LoadAsync *load_async;

    std::list<SubtitleFrame> frames;
    std::list<SubtitleFrame>::iterator itf;
    SubtitleFrame current_frame;
};

void SubtitlePrivate::loadInternal()
{

}

bool SubtitlePrivate::prepareFrame()
{
    SubtitleFrame f;
    if (current_frame.isValid() && current_frame.stop > time_stamp)
        return true;
    while (frames.size() > 0) {
        f = frames.front();
        if (time_stamp < f.start) {
            current_frame.reset();
            return false;
        }
        frames.pop_front();
        if (time_stamp > f.start && time_stamp < f.stop) {
            break;
        }
        f.reset();
    }
    if (f.isValid()) {
        current_frame = f;
        return true;
    }
    current_frame.reset();
    return false;
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
    SubtitleFrame f = d->decoder->processLine(pkt);
    if (!f.isValid())
        return false;
    if (d->frames.empty() || d->frames.back() < f) {
        d->frames.push_back(f);
        d->itf = d->frames.begin();
        return true;
    }
    // usually add to the end. TODO: test
    std::list<SubtitleFrame>::iterator it = d->frames.end();
    if (it != d->frames.begin())
        --it;
    while (it != d->frames.begin() && f < (*it)) { --it; }
    if (it != d->frames.begin()) // found in middle, insert before next
        ++it;
    d->frames.insert(it, f);
    d->itf = it;
    return true;
}

void Subtitle::setTimestamp(double t)
{
    DPTR_D(Subtitle);
    d->time_stamp = t;
    d->prepareFrame();
}

SubtitleFrame Subtitle::frame()
{
    return d_func()->current_frame;
}

NAMESPACE_END
