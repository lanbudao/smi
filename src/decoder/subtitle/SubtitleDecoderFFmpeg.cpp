#include "SubtitleDecoder.h"
#include "Factory.h"
#include "AVDecoder_p.h"
#include "sdk/DPTR.h"
#include "AVLog.h"
#include "mkid.h"

NAMESPACE_BEGIN

StringList supported_sub_extensions_ffmpeg()
{
    StringList codecs;
#if AVFORMAT_STATIC_REGISTER
    const AVInputFormat *i = NULL;
    void* it = NULL;
    while ((i = av_demuxer_iterate(&it))) {
#else
    av_register_all();
    AVInputFormat *i = nullptr;
    while ((i = av_iformat_next(i))) {
#endif
        if (i->long_name && strstr(i->long_name, "subtitle")) {
            if (i->extensions) {
//                Util::push_back(codecs, Util::split(i->extensions, ",", true));
            } else {
                codecs.push_back(std::string(i->name));
            }
        }
    }
    const AVCodec* c = nullptr;
#if AVCODEC_STATIC_REGISTER
    it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        if (c->type == AVMEDIA_TYPE_SUBTITLE)
            codecs.push_back(std::string(c->name));
    }
    const AVCodecDescriptor *desc = nullptr;
    while ((desc = avcodec_descriptor_next(desc))) {
        if (desc->type == AVMEDIA_TYPE_SUBTITLE)
            codecs.push_back(std::string(desc->name));
    }
    codecs.erase(unique(codecs.begin(), codecs.end()), codecs.end());
    return codecs;
}

class SubtitleDecoderFFmpegPrivate: public SubtitleDecoderPrivate
{
public:
    SubtitleDecoderFFmpegPrivate():
        frame(nullptr)
    {
        frame = av_frame_alloc();
    }
    virtual ~SubtitleDecoderFFmpegPrivate()
    {
        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }
    }

    AVFrame *frame;
};

class SubtitleDecoderFFmpeg: public SubtitleDecoder
{
    DPTR_DECLARE_PRIVATE(SubtitleDecoderFFmpeg)
public:
    SubtitleDecoderFFmpeg();
    ~SubtitleDecoderFFmpeg();

    virtual bool open(const std::string &extra);

    StringList supportedCodecs();
    SubtitleFrame processLine(const ByteArray &data, double pts, double duration);

    SubtitleDecoderId id() const;
    std::string description() const;

    bool decode(const Packet& packet);
};

SubtitleDecoderId SubtitleDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER(SubtitleDecoder, FFmpeg, "FFmpeg")

SubtitleDecoderFFmpeg::SubtitleDecoderFFmpeg():
    SubtitleDecoder(new SubtitleDecoderFFmpegPrivate)
{

}

SubtitleDecoderFFmpeg::~SubtitleDecoderFFmpeg()
{

}

bool SubtitleDecoderFFmpeg::open(const string &extra)
{
    DPTR_D(AVDecoder);

    d->opened = false;
    if (!d->codec_ctx) {
        return false;
    }
    AVCodec *codec = findCodec(d->codec_name, d->hwaccel, d->codec_ctx->codec_id);
    if (!codec) {
        //TODO error signal
        return false;
    }
    d->codec_ctx->time_base.num = 1;
    d->codec_ctx->time_base.den = 1000;
    if (!extra.empty()) {
        av_free(d->codec_ctx->extradata);
        d->codec_ctx->extradata = (uint8_t*)av_mallocz(extra.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!d->codec_ctx->extradata)
            return false;
        d->codec_ctx->extradata_size = extra.size();
        memcpy(d->codec_ctx->extradata, extra.c_str(), extra.size());
    }
    if (!d->open()) {
        d->close();
        return false;
    }
    d->opened = true;
    if (avcodec_open2(d->codec_ctx, codec, &d->dict) < 0) {
        avcodec_free_context(&d->codec_ctx);
        d->codec_ctx = nullptr;
        return false;
    }
    return true;
}

StringList SubtitleDecoderFFmpeg::supportedCodecs()
{
    return supported_sub_extensions_ffmpeg();
}

SubtitleFrame SubtitleDecoderFFmpeg::processLine(Packet& pkt)
{
    DPTR_D(SubtitleDecoderFFmpeg);

    if (!d->codec_ctx)
        return SubtitleFrame();
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.size = data.size();
    pkt.data = (uint8_t *)data.constData();
    const double unit = 1.0 / av_q2d(d->codec_ctx->time_base);
    pkt.pts = pts * unit;
    pkt.duration = duration * unit;
    AVSubtitle sub;
    memset(&sub, 0, sizeof(sub));
    int got_subtitle = 0;
    int ret = avcodec_decode_subtitle2(d->codec_ctx, &sub, &got_subtitle, &pkt);
    if (ret < 0 || !got_subtitle) {
        av_packet_unref(&pkt);
        avsubtitle_free(&sub);
        return SubtitleFrame();
    }
    SubtitleFrame frame;
    frame.start = pts + (double)sub.start_display_time / 1000;
    frame.stop = pts + (double)sub.end_display_time / 1000;
    for (unsigned i = 0; i < sub.num_rects; i++) {
        switch (sub.rects[i]->type) {
        case SUBTITLE_ASS:
            AVDebug("ass subtitle frame: %s\n", sub.rects[i]->ass);
//            frame.text.append(PlainText::fromAss(sub.rects[i]->ass)).append(ushort('\n'));
            break;
        case SUBTITLE_TEXT:
            AVDebug("text subtitle frame: %s\n", sub.rects[i]->text);
            frame.text.append(sub.rects[i]->text);
            frame.text.append("\n");
            break;
        case SUBTITLE_BITMAP:
            break;
        default:
            break;
        }
    }
    av_packet_unref(&pkt);
    avsubtitle_free(&sub);
    return frame;
}

SubtitleDecoderId SubtitleDecoderFFmpeg::id() const
{
    return SubtitleDecoderId_FFmpeg;
}

std::string SubtitleDecoderFFmpeg::description() const
{
    return "";
}

bool SubtitleDecoderFFmpeg::decode(const Packet &pkt)
{

    return true;
}

NAMESPACE_END
