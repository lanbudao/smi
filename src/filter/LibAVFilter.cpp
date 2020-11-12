#include "sdk/filter/LibAVFilter.h"
#include "AudioFrame.h"
#include "VideoFrame.h"
#include "inner.h"
#include "sdk/AVLog.h"
#include "Filter_p.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/avstring.h"
}
#include <assert.h>
#include <sstream>
NAMESPACE_BEGIN

#define AV_FRAME_REF 0

#define FFPROC_HAVE_av_buffersink_get_frame (LIBAV_MODULE_CHECK(LIBAVFILTER, 4, 2, 0) || FFMPEG_MODULE_CHECK(LIBAVFILTER, 3, 79, 100)) //3.79.101: ff2.0.4

// local types can not be used as template parameters
class AVFrameHolder {
public:
    AVFrameHolder() {
        m_frame = av_frame_alloc();
#if !FFPROC_HAVE_av_buffersink_get_frame
        picref = 0;
#endif
    }
    ~AVFrameHolder() {
        av_frame_free(&m_frame);
#if !FFPROC_HAVE_av_buffersink_get_frame
        avfilter_unref_bufferp(&picref);
#endif
    }
    AVFrame* frame() { return m_frame; }
#if !FFPROC_HAVE_av_buffersink_get_frame
    AVFilterBufferRef** bufferRef() { return &picref; }
    // copy properties and data ptrs(no deep copy).
    void copyBufferToFrame() { avfilter_copy_buf_props(m_frame, picref); }
#endif
private:
    AVFrame *m_frame;
#if !FFPROC_HAVE_av_buffersink_get_frame
    AVFilterBufferRef *picref;
#endif
};
typedef std::shared_ptr<AVFrameHolder> AVFrameHolderRef;

class LibAVFilterPrivate
{
public:
    LibAVFilterPrivate() :
        av_frame(nullptr),
        status(LibAVFilter::NotConfigured)
    {
        filter_graph = 0;
        in_filter_ctx = 0;
        out_filter_ctx = 0;
        avfilter_register_all();
    }
    ~LibAVFilterPrivate()
    {
        avfilter_graph_free(&filter_graph);
        if (av_frame) {
            av_frame_free(&av_frame);
            av_frame = nullptr;
        }
    }
    bool config(const std::string &args, bool is_video);

public:
    AVFrame* av_frame;
    std::string options;
    LibAVFilter::Status status;
    AVFilterGraph *filter_graph;
    AVFilterContext *in_filter_ctx;
    AVFilterContext *out_filter_ctx;
};

bool LibAVFilterPrivate::config(const std::string &args, bool is_video)
{
    if (av_frame) {
        av_frame_free(&av_frame);
        av_frame = nullptr;
    }
    status = LibAVFilter::ConfigureFailed;

    avfilter_graph_free(&filter_graph);
    filter_graph = avfilter_graph_alloc();
    AVDebug("buffersrc_args=%s\n", args.data());
#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(7,0,0)
    const
#endif
    AVFilter *buffersrc = avfilter_get_by_name(is_video ? "buffer" : "abuffer");
    assert(buffersrc);
    AV_ENSURE_OK(avfilter_graph_create_filter(&in_filter_ctx,
        buffersrc,
        "in", args.data(), NULL,
        filter_graph)
        , false);
    /* buffer video sink: to terminate the filter chain. */
#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(7,0,0)
    const
#endif
    AVFilter *buffersink = avfilter_get_by_name(is_video ? "buffersink" : "abuffersink");
    assert(buffersink);
    AV_ENSURE_OK(avfilter_graph_create_filter(&out_filter_ctx, buffersink, "out",
        NULL, NULL, filter_graph)
        , false);
    /* Endpoints for the filter graph. */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    outputs->name = av_strdup("in");
    outputs->filter_ctx = in_filter_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = out_filter_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    struct delete_helper {
        AVFilterInOut **x;
        delete_helper(AVFilterInOut **io) : x(io) {}
        ~delete_helper() {
            // libav always free it in avfilter_graph_parse. so we does nothing
#if FFPRO_USE_FFMPEG(LIBAVFILTER)
            avfilter_inout_free(x);
#endif
        }
    } scoped_in(&inputs), scoped_out(&outputs);
    //avfilter_graph_parse, avfilter_graph_parse2?
    AV_ENSURE_OK(avfilter_graph_parse_ptr(filter_graph, options.data(), &inputs, &outputs, NULL), false);
    AV_ENSURE_OK(avfilter_graph_config(filter_graph, NULL), false);
    av_frame = av_frame_alloc();
    status = LibAVFilter::ConfigureOk;
    return true;
}

LibAVFilter::LibAVFilter():
    priv(new LibAVFilterPrivate())
{

}

LibAVFilter::~LibAVFilter()
{
    delete priv;
}

void LibAVFilter::setOptions(const std::string & opt)
{
    if (priv->options == opt)
        return;
    priv->options = opt;
    priv->status = LibAVFilter::NotConfigured;
}

std::string LibAVFilter::options() const
{
    return priv->options;
}

LibAVFilter::Status LibAVFilter::status() const
{
    return priv->status;
}

bool LibAVFilter::putFrame(Frame * frame, bool changed, bool is_video)
{
    if (priv->status == LibAVFilter::NotConfigured || !priv->av_frame || changed) {
        if (!priv->config(sourceArguments(), is_video)) {
            AVWarning("setup audio filter graph error\n");
            return false;
        }
    }
    //av_frame_ref(priv->av_frame, frame->frame());
    //AV_ENSURE_OK(av_buffersrc_write_frame(priv->in_filter_ctx, priv->av_frame), false);
    //av_frame_unref(priv->av_frame);
    /**
     * If use av_buffersrc_write_frame, we must call av_frame_unref after write.
     * because it will creates a new reference to the input frame.
     */
    AV_ENSURE_OK(av_buffersrc_add_frame(priv->in_filter_ctx, frame->frame()), false);
    return true; 
}

bool LibAVFilter::getFrame()
{
    int ret = av_buffersink_get_frame(priv->out_filter_ctx, priv->av_frame);
    if (ret < 0) {
        AVWarning("av_buffersink_get_frame error: %s\n", averror2str(ret));
        return false;
    }
    return true;
}

void * LibAVFilter::getFrameHolder()
{
#if FFPROC_HAVE_AVFILTER
    AVFrameHolder *holder = NULL;
    holder = new AVFrameHolder();
#if FFPROC_HAVE_av_buffersink_get_frame
    int ret = av_buffersink_get_frame(priv->out_filter_ctx, holder->frame());
#else
    int ret = av_buffersink_read(priv->out_filter_ctx, holder->bufferRef());
#endif //QTAV_HAVE_av_buffersink_get_frame
    if (ret < 0) {
        AVWarning("av_buffersink_get_frame error: %s\n", averror2str(ret));
        delete holder;
        return 0;
    }
#if !FFPROC_HAVE_av_buffersink_get_frame
    holder->copyBufferToFrame();
#endif
    return holder;
#endif
    return 0;
}

class LibAVFilterAudioPrivate: public AudioFilterPrivate
{
public:
    LibAVFilterAudioPrivate()
        : AudioFilterPrivate()
        , sample_rate(0)
        , sample_fmt(AV_SAMPLE_FMT_NONE)
        , channel_layout(0)
    {}
    int sample_rate;
    AVSampleFormat sample_fmt;
    int64_t channel_layout;
};

LibAVFilterAudio::LibAVFilterAudio():
    AudioFilter(new LibAVFilterAudioPrivate)
{

}

LibAVFilterAudio::~LibAVFilterAudio()
{

}

bool LibAVFilterAudio::process(MediaInfo * info, AudioFrame * aframe)
{
    if (status() == ConfigureFailed)
        return false;
    DPTR_D(LibAVFilterAudio);
    bool changed = false;
    const AudioFormat afmt(aframe->format());
    if (d->sample_rate != afmt.sampleRate() ||
        d->sample_fmt != afmt.sampleFormatFFmpeg() ||
        d->channel_layout != afmt.channelLayoutFFmpeg()) {
        changed = true;
        d->sample_rate = afmt.sampleRate();
        d->sample_fmt = (AVSampleFormat)afmt.sampleFormatFFmpeg();
        d->channel_layout = afmt.channelLayoutFFmpeg();
    }
    if (!putFrame(aframe, changed))
        return false;
#if 0
    AVFrameHolderRef ref((AVFrameHolder*)getFrameHolder());
    if (!ref)
        return false;
    const AVFrame *f = ref->frame();
#else
    AVFrame* f = aframe->frame();
    int ret = av_buffersink_get_frame(priv->out_filter_ctx, f);
    if (ret < 0) {
        AVWarning("av_buffersink_get_frame error: %s\n", averror2str(ret));
        return false;
    }
#endif
    AudioFormat fmt;
    fmt.setSampleFormatFFmpeg(f->format);
    fmt.setChannelLayoutFFmpeg(f->channel_layout);
    fmt.setSampleRate(f->sample_rate);
    if (!fmt.isValid()) {// need more data to decode to get a frame
        return false;
    }
    aframe->setFormat(fmt);
    aframe->setBits(f->extended_data);
    aframe->setBytesPerLine(f->linesize[0], 0);
    aframe->setSamplePerChannel(f->nb_samples);
    AVRational tb = av_buffersink_get_time_base(priv->out_filter_ctx);
    aframe->setTimestamp(f->pts == AV_NOPTS_VALUE ? NAN : f->pts * av_q2d(tb));
    return true;
}

std::string LibAVFilterAudio::sourceArguments() const
{
    DPTR_D(const LibAVFilterAudio);
    std::string args;
    stringstream ss;
    ss << "time_base=1/" << d->sample_rate;
    ss << ":sample_rate=" << d->sample_rate;
    ss << ":sample_fmt=" << d->sample_fmt;
    ss << ":channel_layout=0x" << std::hex << d->channel_layout;
    args = ss.str();
    return args;
}

/**
 * @brief libavfiltervideo
 */
class LibAVFilterVideoPrivate : public VideoFilterPrivate
{
public:
    LibAVFilterVideoPrivate()
        : VideoFilterPrivate(),
        format(AV_PIX_FMT_NONE),
        width(0),
        height(0)
    {}

    AVPixelFormat format;
    int width, height;
    Rational sample_aspect_ratio, time_base;
    Rational frame_rate;
};

LibAVFilterVideo::LibAVFilterVideo() :
    VideoFilter(new LibAVFilterVideoPrivate)
{

}

LibAVFilterVideo::~LibAVFilterVideo()
{

}

bool LibAVFilterVideo::process(MediaInfo * info, VideoFrame * vframe)
{
    if (status() == ConfigureFailed)
        return false;
    DPTR_D(LibAVFilterVideo);
    bool changed = false;
    if (d->width != vframe->width() || d->height != vframe->height() || d->format != vframe->pixelFormatFFmpeg()) {
        changed = true;
        d->width = vframe->width();
        d->height = vframe->height();
        d->format = (AVPixelFormat)vframe->pixelFormatFFmpeg();
        if (info && info->video) {
            d->sample_aspect_ratio = info->video->sample_aspect_ratio;
            d->time_base = info->video->time_base;
            d->frame_rate = info->video->frame_rate;
        }
    }
    if (!putFrame(vframe, changed, true))
        return false;
    AVFrame* f = vframe->frame();
    int ret = av_buffersink_get_frame(priv->out_filter_ctx, f);
    if (ret < 0) {
        AVWarning("av_buffersink_get_frame error: %s\n", averror2str(ret));
        return false;
    }
    vframe->setWidth(f->width);
    vframe->setHeight(f->height);
    vframe->setBits(f->data);
    vframe->setFormat(VideoFormat(f->format));
    vframe->setBytesPerLine((int*)f->linesize);
    AVRational tb = av_buffersink_get_time_base(priv->out_filter_ctx);
    vframe->setTimestamp(f->pts == AV_NOPTS_VALUE ? NAN : f->pts * av_q2d(tb));
    return true;
}

std::string LibAVFilterVideo::sourceArguments() const
{
    DPTR_D(const LibAVFilterVideo);
    std::string args;
    char buffersrc_args[256];
    snprintf(buffersrc_args, sizeof(buffersrc_args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
        d->width, d->height, d->format,
        d->time_base.num, d->time_base.den,
        d->sample_aspect_ratio.num, FFMAX(d->sample_aspect_ratio.den, 1));
    if (d->frame_rate.num && d->frame_rate.den)
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", d->frame_rate.num, d->frame_rate.den);
    args = buffersrc_args;
    return args;
}
NAMESPACE_END

