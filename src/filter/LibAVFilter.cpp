#include "sdk/filter/LibAVFilter.h"
#include "AudioFrame.h"
#include "VideoFrame.h"
#include "inner.h"
#include "AVLog.h"
#include "Filter_p.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
}
#include <assert.h>
#include <sstream>
NAMESPACE_BEGIN

class LibAVFilterPrivate
{
public:
    LibAVFilterPrivate() :
        status(LibAVFilter::NotConfigured),
        sws_dict(nullptr)
    {
        av_dict_set(&sws_dict, "flags", "bicubic", 0);
        filter_graph = 0;
        in_filter_ctx = 0;
        out_filter_ctx = 0;
        avfilter_register_all();
    }
    ~LibAVFilterPrivate()
    {
        av_dict_free(&sws_dict);
        avfilter_graph_free(&filter_graph);
    }
    bool config_filter_graph();
    bool config_filter(const std::string &args, bool is_video);

public:
    std::string options;
    AVDictionary *sws_dict;
    LibAVFilter::Status status;
    AVFilterGraph *filter_graph;
    AVFilterContext *in_filter_ctx;
    AVFilterContext *out_filter_ctx;
};

bool LibAVFilterPrivate::config_filter_graph()
{
    int i = 0;
    int nb_filters;

    nb_filters = filter_graph->nb_filters;

    struct DeleteHelper {
        DeleteHelper(AVFilterInOut** inout_) :inout(inout_) {}
        ~DeleteHelper() { avfilter_inout_free(inout); }
        AVFilterInOut** inout;
    };
    AVFilterInOut *outputs = nullptr, *inputs = nullptr;
    if (filter_graph) {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        DeleteHelper scope_out(&outputs), scope_in(&inputs);
        if (!outputs || !inputs) {
            return false;
        }
        outputs->name = av_strdup("in");
        outputs->filter_ctx = in_filter_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = out_filter_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        AV_ENSURE_OK(avfilter_graph_parse_ptr(filter_graph, options.data(), &inputs, &outputs, NULL), false);
    }
    else {
        AV_ENSURE_OK(avfilter_link(in_filter_ctx, 0, out_filter_ctx, 0), false);
    }
    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
    for (i = 0; i < filter_graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext*, filter_graph->filters[i], filter_graph->filters[i + nb_filters]);
    AV_ENSURE_OK(avfilter_graph_config(filter_graph, NULL), false);
    return true;
}

bool LibAVFilterPrivate::config_filter(const std::string &args, bool video)
{
    status = LibAVFilter::ConfigureFailed;

    avfilter_graph_free(&filter_graph);
    filter_graph = avfilter_graph_alloc();
    AVDebug("buffersrc_args=%s\n", args.data());
    if (video) {
        char sws_flags_str[512] = "";
        AVDictionaryEntry *e = NULL;
        while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
            if (!strcmp(e->key, "sws_flags")) {
                av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
            }
            else
                av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
        }
        if (strlen(sws_flags_str))
            sws_flags_str[strlen(sws_flags_str) - 1] = '\0';
        filter_graph->scale_sws_opts = av_strdup(sws_flags_str);
    }
    const AVFilter *buffersrc = avfilter_get_by_name(video ? "buffer" : "abuffer");
    assert(buffersrc);
    AV_ENSURE_OK(avfilter_graph_create_filter(&in_filter_ctx,
        buffersrc,
        "in", args.data(), NULL,
        filter_graph)
        , false);
    /* buffer video sink: to terminate the filter chain. */
    const AVFilter *buffersink = avfilter_get_by_name(video ? "buffersink" : "abuffersink");
    assert(buffersink);
    AV_ENSURE_OK(avfilter_graph_create_filter(&out_filter_ctx, buffersink, "out",
        NULL, NULL, filter_graph)
        , false);
    // TODO: auto rotate, see ffplay
    
    if (!config_filter_graph())
        return false;

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

bool LibAVFilter::configure(bool video)
{
    return priv->config_filter(sourceArguments(), video);
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
    if (status() == LibAVFilter::NotConfigured || changed) {
        if (!configure()) {
            AVWarning("Configure audio filter failed!\n");
            return false;
        }
    }
    /**
     * If use av_buffersrc_write_frame, we must call av_frame_unref after write.
     * because it will creates a new reference to the input frame.
     */
    AVFrame* f = aframe->frame();
    AV_ENSURE_OK(av_buffersrc_add_frame(priv->in_filter_ctx, f), false);
    AV_ENSURE_OK(av_buffersink_get_frame(priv->out_filter_ctx, f), false);

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

void LibAVFilterVideo::setSwsOpts(std::map<std::string, std::string> &opts)
{
    std::map<std::string, std::string>::iterator it = opts.begin();
    for (; it != opts.end(); ++it) {
        av_dict_set(&priv->sws_dict, it->first.c_str(), it->second.c_str(), AV_DICT_APPEND);
    }
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
    if (status() == LibAVFilter::NotConfigured || changed) {
        if (!configure(true)) {
            AVWarning("Configure video filter failed!\n");
            return false;
        }
    }
    AVFrame* f = vframe->frame();
    AV_ENSURE_OK(av_buffersrc_add_frame(priv->in_filter_ctx, f), false);
    AV_ENSURE_OK(av_buffersink_get_frame(priv->out_filter_ctx, f), false);

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