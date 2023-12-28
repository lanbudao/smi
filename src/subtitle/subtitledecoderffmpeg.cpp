#include "SubtitleDecoder.h"
#include "demuxer/Demuxer.h"
#include "AVLog.h"
#include "utils/stringaide.h"
#include "inner.h"
#include "plaintext.h"
#include "AVDecoder_p.h"
#include "subtitle/subtitleframe.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

NAMESPACE_BEGIN

class SubtitleDecoderFFmpegPrivate : public SubtitleDecoderPrivate
{
public:
    SubtitleDecoderFFmpegPrivate()
    {

    }
    virtual ~SubtitleDecoderFFmpegPrivate()
    {

    }

};

class SubtitleDecoderFFmpeg: public SubtitleDecoder
{
    DPTR_DECLARE_PRIVATE(SubtitleDecoderFFmpeg)
public:
    SubtitleDecoderFFmpeg();
    ~SubtitleDecoderFFmpeg();

    SubtitleDecoderId id() const;
    std::string name() const;
    std::list<std::string> supportedTypes() const;

    bool process(const std::string& path);
    std::list<SubtitleFrame> frames() const;
    bool processHeader(MediaInfo *info);
    SubtitleFrame processLine(Packet *pkt);
    std::string getText(double pts) const;

    virtual void onOpen();
    virtual SubtitleFrame decode(Packet *pkt, int *ret);

private:
    bool processSubtitle();
    AVCodecContext *codec_ctx;
    Demuxer sub_reader;
    std::list<SubtitleFrame> m_frames;
    SubtitleFrame frame;//current frame
    AVSubtitle av_subtitle;
    MediaInfo *media_info;
};

extern SubtitleDecoderId SubtitleDecoderId_FFmpeg;
FACTORY_REGISTER(SubtitleDecoder, FFmpeg, "FFmpeg")

SubtitleDecoderFFmpeg::SubtitleDecoderFFmpeg():
    SubtitleDecoder(new SubtitleDecoderFFmpegPrivate),
    codec_ctx(nullptr),
    media_info(nullptr)
{
    memset(&av_subtitle, 0, sizeof(av_subtitle));
}

SubtitleDecoderFFmpeg::~SubtitleDecoderFFmpeg()
{
    avcodec_free_context(&codec_ctx);
}

SubtitleDecoderId SubtitleDecoderFFmpeg::id() const
{
    return SubtitleDecoderId_FFmpeg;
}

std::string SubtitleDecoderFFmpeg::name() const
{
    return "FFmpeg";
}

std::list<std::string> ffmpeg_supported_sub_extensions_by_codec()
{
    std::list<std::string> exts;
    const AVCodec* c = NULL;
#if AVCODEC_STATIC_REGISTER
    void* it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        if (c->type != AVMEDIA_TYPE_SUBTITLE)
            continue;
        AVDebug("sub codec: %s\n", c->name);
#if AVFORMAT_STATIC_REGISTER
        const AVInputFormat *i = NULL;
        void* it2 = NULL;
        while ((i = av_demuxer_iterate(&it2))) {
#else
        av_register_all(); // MUST register all input/output formats
        AVInputFormat *i = NULL;
        while ((i = av_iformat_next(i))) {
#endif
            if (!strcmp(i->name, c->name)) {
                AVDebug("found iformat\n");
                if (i->extensions) {
                    exts.splice(exts.end(), StringAide::split_list(i->extensions, ","));
                }
                else {
                    exts.push_back(i->name);
                }
                break;
            }
        }
        if (!i) {
            //qDebug("codec name '%s' is not found in AVInputFormat, just append codec name", c->name);
            //exts.append(c->name);
        }
        }
    return exts;
    }

std::list<std::string> ffmpeg_supported_sub_extensions()
{
    std::list<std::string> exts;
#if AVFORMAT_STATIC_REGISTER
    const AVInputFormat *i = NULL;
    void* it = NULL;
    while ((i = av_demuxer_iterate(&it))) {
#else
    av_register_all(); // MUST register all input/output formats
    AVInputFormat *i = NULL;
    while ((i = av_iformat_next(i))) {
#endif
        // strstr parameters can not be null
        if (i->long_name && strstr(i->long_name, "subtitle")) {
            if (i->extensions) {
                exts.splice(exts.end(), StringAide::split_list(i->extensions, ","));
            }
            else {
                exts.push_back(i->name);
            }
        }
    }
    // AVCodecDescriptor.name and AVCodec.name may be different. avcodec_get_name() use AVCodecDescriptor if possible
    std::list<std::string> codecs;
    const AVCodec* c = NULL;
#if AVCODEC_STATIC_REGISTER
    it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        if (c->type == AVMEDIA_TYPE_SUBTITLE)
            codecs.push_back(c->name);
    }
    const AVCodecDescriptor *desc = NULL;
    while ((desc = avcodec_descriptor_next(desc))) {
        if (desc->type == AVMEDIA_TYPE_SUBTITLE)
            codecs.push_back(desc->name);
    }
    exts.splice(exts.end(), codecs);
    exts.unique();
    return exts;
}

std::list<std::string> SubtitleDecoderFFmpeg::supportedTypes() const
{
    static const std::list<std::string> sSuffixes = ffmpeg_supported_sub_extensions();
    return sSuffixes;
}

bool SubtitleDecoderFFmpeg::process(const std::string &path)
{
    struct UnloadAide {
        UnloadAide(Demuxer* d) :demuxer(d) {}
        ~UnloadAide() { demuxer->unload(); }
        Demuxer* demuxer;
    };
    sub_reader.setMedia(path);
    UnloadAide aide(&sub_reader);
    if (!sub_reader.load())
        return false;
    if (sub_reader.streamIndex(MediaTypeSubtitle) < 0)
        return false;
    if (!processSubtitle())
        return false;
    return true;
}

std::list<SubtitleFrame> SubtitleDecoderFFmpeg::frames() const
{
    return m_frames;
}

std::string SubtitleDecoderFFmpeg::getText(double pts) const
{
    std::string text;
    //for (int i = 0; i < m_frames.size(); ++i) {
    //    if (m_frames[i].begin <= pts && m_frames[i].end >= pts) {
    //        text += m_frames[i].text + std::list<std::string>("\n");
    //        continue;
    //    }
    //    if (!text.isEmpty())
    //        break;
    //}
    return text;
}

bool SubtitleDecoderFFmpeg::processHeader(MediaInfo *info)
{
    if (!info && !info->subtitle)
        return false;
    media_info = info;
    std::string codec = info->subtitle->codec_name;
    uint8_t *data = info->subtitle->extradata;
    int size = info->subtitle->extradata_size;
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
    }
    AVCodec *c = avcodec_find_decoder_by_name(codec.c_str());
    if (!c) {
        AVDebug("subtitle avcodec_descriptor_get_by_name %s\n", codec.c_str());
        const AVCodecDescriptor *desc = avcodec_descriptor_get_by_name(codec.c_str());
        if (!desc) {
            AVWarning("No codec descriptor found for %s\n", codec.c_str());
            return false;
        }
        c = avcodec_find_decoder(desc->id);
    }
    if (!c) {
        AVWarning("No subtitle decoder found for codec: %s, try fron descriptor\n", codec.c_str());
        return false;
    }
    codec_ctx = avcodec_alloc_context3(c);
    if (!codec_ctx)
        return false;
    // no way to get time base. the pts unit used in processLine() is 's', ffmpeg use ms, so set 1/1000 here
    codec_ctx->time_base.num = info->subtitle->time_base.num;
    codec_ctx->time_base.den = info->subtitle->time_base.den;
    if (size > 0) {
        av_free(codec_ctx->extradata);
        codec_ctx->extradata = (uint8_t*)av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!codec_ctx->extradata)
            return false;
        codec_ctx->extradata_size = size;
        memcpy(codec_ctx->extradata, data, size);
    }
    if (avcodec_open2(codec_ctx, c, NULL) < 0) {
        avcodec_free_context(&codec_ctx);
        return false;
    }
    return true;//codec != QByteArrayLiteral("ass") && codec != QByteArrayLiteral("ssa");
}

SubtitleFrame SubtitleDecoderFFmpeg::processLine(Packet *pkt)
{
    if (!codec_ctx)
        return SubtitleFrame();
    AVPacket* packet = pkt->avPacket();
    AVSubtitle sub;
    memset(&sub, 0, sizeof(sub));
    int got_subtitle = 0;
    int ret = avcodec_decode_subtitle2(codec_ctx, &sub, &got_subtitle, packet);
    if (ret < 0 || !got_subtitle) {
        avsubtitle_free(&sub);
        return SubtitleFrame();
    }
    SubtitleFrame frame;
    //frame.type = (sub.format == 0) ? SubtitlePixmap : SubtitleText;
    // relative to packet pts, in ms
    double pts;
    if (sub.pts != AV_NOPTS_VALUE && media_info && media_info->subtitle)
        pts = sub.pts / av_q2d(
            {
                media_info->subtitle->time_base.num,
                media_info->subtitle->time_base.den,
            });
    else
        pts = pkt->pts;
    frame.start = pts + FORCE_DOUBLE(sub.start_display_time) / 1000;
    frame.end = pts + FORCE_DOUBLE(sub.end_display_time) / 1000;

    //for (unsigned i = 0; i < sub.num_rects; i++) {
    //    switch (sub.rects[i]->type) {
    //    case SUBTITLE_ASS:
    //        //qDebug("ass frame: %s", sub.rects[i]->ass);
    //        frame.push_back(PlainText::fromAss(sub.rects[i]->ass));
    //        break;
    //    case SUBTITLE_TEXT:
    //        //qDebug("txt frame: %s", sub.rects[i]->text);
    //        frame.push_back(sub.rects[i]->text);
    //        break;
    //    case SUBTITLE_BITMAP:
    //        //sub.rects[i]->w > 0 && sub.rects[i]->h > 0
    //        //AVDebug("bmp sub");
    //        frame = SubtitleFrame(); // not support bmp subtitle now
    //        break;
    //    default:
    //        break;
    //    }
    //}
    //avsubtitle_free(&sub);
    return frame;
}

void SubtitleDecoderFFmpeg::onOpen()
{

}

SubtitleFrame SubtitleDecoderFFmpeg::decode(Packet *pkt, int *ret)
{
    DPTR_D(SubtitleDecoderFFmpeg);
    AVPacket* packet = pkt->avPacket();
    SubtitleFrame frame;
    AVSubtitle *av_subtitle = frame.data();

    int got_subtitle = 0;
    *ret = avcodec_decode_subtitle2(d->codec_ctx, av_subtitle, &got_subtitle, packet);
    if (*ret < 0 || !got_subtitle) {
        return frame;
    }
    // relative to packet pts, in ms
    double pts;
    if (av_subtitle->pts != AV_NOPTS_VALUE && media_info && media_info->subtitle)
        pts = av_subtitle->pts / av_q2d(
            {
                media_info->subtitle->time_base.num,
                media_info->subtitle->time_base.den,
            });
    else
        pts = pkt->pts;
    frame.start = pts + FORCE_DOUBLE(av_subtitle->start_display_time) / 1000;
    frame.end = pts + FORCE_DOUBLE(av_subtitle->end_display_time) / 1000;
    frame.setSerial(pkt->serial);
    return frame;
}

bool SubtitleDecoderFFmpeg::processSubtitle()
{
    m_frames.clear();
    codec_ctx = sub_reader.stream(MediaTypeSubtitle)->codec;
    AVCodecID codec_id = sub_reader.stream(MediaTypeSubtitle)->codecpar->codec_id;
    AVCodec *dec = avcodec_find_decoder(codec_id);
    const AVCodecDescriptor *dec_desc = avcodec_descriptor_get(codec_id);
    if (!dec) {
        if (dec_desc)
            AVWarning("Failed to find subtitle codec %s\n", dec_desc->name);
        else
            AVWarning("Failed to find subtitle codec %d\n", codec_id);
        return false;
    }
    AVDebug("found subtitle decoder '%s'\n", dec_desc->name);
    // AV_CODEC_PROP_TEXT_SUB: ffmpeg >= 2.0
#ifdef AV_CODEC_PROP_TEXT_SUB
    if (dec_desc && !(dec_desc->props & AV_CODEC_PROP_TEXT_SUB)) {
        AVWarning("Only text based subtitles are currently supported\n");
        return false;
    }
#endif
    AVDictionary *codec_opts = NULL;
    int ret = avcodec_open2(sub_reader.stream(MediaTypeSubtitle)->codec, dec, &codec_opts);
    if (ret < 0) {
        AVWarning("open subtitle codec error: %s\n", averror2str(ret));
        av_dict_free(&codec_opts);
        return false;
    }
    int stream = sub_reader.streamIndex(MediaTypeSubtitle);
    while (!sub_reader.atEnd()) {
        if (!sub_reader.readFrame()) { // eof or other errors
            continue;
        }
        if (sub_reader.stream() != stream)
            continue;
        Packet pkt = sub_reader.packet();
        if (!pkt.isValid())
            continue;
        SubtitleFrame frame = processLine(&pkt);
        if (frame.valid())
            m_frames.push_back(frame);
    }
    avcodec_close(codec_ctx);
    codec_ctx = nullptr;
    return true;
}

NAMESPACE_END