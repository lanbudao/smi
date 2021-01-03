#include "subtitle.h"
#include "SubtitleDecoder.h"
#include "AVLog.h"
#include "CThread.h"
#include "util/mkid.h"
#include "util/CThread.h"
#include "Demuxer.h"
#include "BlockQueue.h"
#include "inner.h"
#include "subtitle/subtitleframe.h"
#include "subtitle/assrender.h"
extern "C" {
#include "libavcodec/avcodec.h"
}
NAMESPACE_BEGIN

#define SUB_QUEUE_SIZE 16

typedef std::list<SubtitleFrame> SubtitleQueue;
class SubtitleReadThread : public CThread
{
public:
    SubtitleReadThread() :
        abort(false),
        sbuffer(nullptr),
        status(NoMedia),
        decoder(nullptr)
    {
    }
    ~SubtitleReadThread() {}

    void setBuffer(SubtitleQueue *buffer) { sbuffer = buffer; }
    void setFile(const std::string &file) { file_name = file; }
    void setLoadedCallback(std::function<void()> f) { loadedCallback = f; }
    SubtitleDecoder *subtitleDecoder() { return decoder; }
    void run();
    void stop();
    MediaStatus mediaStatus() { return status; }

private:
    bool abort;
    Demuxer demuxer;
    std::string file_name;
    SubtitleQueue *sbuffer;
    MediaInfo info;
    MediaStatus status;
    SubtitleDecoder *decoder;
    std::function<void()> loadedCallback;
};

void SubtitleReadThread::stop()
{
    abort = true;
}

void SubtitleReadThread::run()
{
    int ret;
    int stream;
    Packet pkt;

    status = Unloaded;
    if (!sbuffer)
        return;
    demuxer.setMediaInfo(&info);
    demuxer.setMedia(file_name);
    status = Loading;
    if (demuxer.load() < 0) {
        status = Invalid;
        return;
    }
    decoder = SubtitleDecoder::create(SubtitleDecoderId_FFmpeg);
    if (!decoder)
        return;
    if (FFMPEG_MODULE_CHECK(LIBAVCODEC, 57, 26, 100)) {
        std::map < std::string, std::string > options;
        options.insert(std::make_pair("sub_text_format", "ass"));
        decoder->setCodeOptions(options);
    }
    decoder->initialize(demuxer.formatCtx(), demuxer.stream(MediaTypeSubtitle));
    if (!decoder->open())
        return;
    while (!abort) {
        ret = demuxer.readFrame();
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                break;
            }
            continue;
        }
        stream = demuxer.stream();
        pkt = demuxer.packet();

        if (stream == demuxer.streamIndex(MediaTypeSubtitle)) {
            SubtitleFrame frame = decoder->decode(&pkt, &ret);
            if (!frame.valid()) {
                continue;
            }
            /* 0 = graphics */
            //if (frame.data()->format != 0) {
            //    ass_render.addSubtitleToTrack(frame.data());
            //}
            sbuffer->push_back(frame);
        }
    }
    auto compare = [=](const SubtitleFrame &f1, const SubtitleFrame &f2)->bool {
        return f1.start <= f2.start;
    };
    sbuffer->sort(compare);
    CALL_BACK(loadedCallback);
    status = Loaded;
}

class SubtitlePrivate
{
public:
    SubtitlePrivate():
        enabled(true),
        time_stamp(0),
        read_thread(nullptr)
    {

    }
    virtual ~SubtitlePrivate()
    {
        unload();
    }

    bool prepareFrame();

    void unload()
    {
        if (read_thread) {
            read_thread->stop();
            read_thread->wait();
            delete read_thread;
            read_thread = nullptr;
        }
    }

    void onloaded()
    {
        itf = frames.begin();
    }

    bool enabled;
    std::string file_name;
    std::string codec;
    double time_stamp;

    SubtitleFrame current_frame;

    SubtitleReadThread *read_thread;
    SubtitleQueue frames;
    SubtitleQueue::iterator itf;

    ASSAide::ASSRender ass_render;
};

bool SubtitlePrivate::prepareFrame()
{
    if (!read_thread || !(read_thread->mediaStatus() == Loaded))
        return false;
    if (time_stamp >= current_frame.start && time_stamp <= current_frame.end)
        return true;
    current_frame.reset();
    SubtitleQueue::iterator it = itf;
    for (; it != frames.end(); ++it) {
        if (time_stamp >= it->start) {
            if (time_stamp <= it->end) {
                current_frame = *it;
                if (ass_render.initialized()) {
                    ass_render.addSubtitleToTrack(current_frame.data());
                }
                itf = it;
                return true;
            }
        }
        else {
            break;
        }
    }
    return false;
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
    d->file_name = name;
}

void Subtitle::setCodec(const std::string &codec)
{
    DPTR_D(Subtitle);
    d->codec = codec;
}

void Subtitle::load()
{
    DPTR_D(Subtitle);

    d->unload();
    d->read_thread = new SubtitleReadThread;
    d->read_thread->setBuffer(&d->frames);
    d->read_thread->setFile(d->file_name);
    auto onload = [d]()->void {d->onloaded(); };
    d->read_thread->setLoadedCallback(onload);
    d->read_thread->start();
}

void Subtitle::processSeek()
{
    DPTR_D(Subtitle);
    if (!d->frames.empty())
        d->itf = d->frames.begin();
}

void Subtitle::setTimestamp(double t, int w, int h)
{
    DPTR_D(Subtitle);
    d->time_stamp = t;
    if (!d->ass_render.initialized() && d->read_thread->subtitleDecoder()) {
        d->ass_render.initialize(d->read_thread->subtitleDecoder()->codecCtx(), w, h);
    }
    d->prepareFrame();
}

SubtitleFrame* Subtitle::frame()
{
    return &d_func()->current_frame;
}

NAMESPACE_END
