#include "SubtitleDecoder.h"
#include "Demuxer.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#include "sdk/AVLog.h"
#include "util/stringaide.h"
#include "inner.h"
#include "plaintext.h"

NAMESPACE_BEGIN

class SubtitleDecoderFFmpeg: public SubtitleDecoder
{
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

private:
    bool processSubtitle();
    AVCodecContext *codec_ctx;
    Demuxer sub_reader;
    std::list<SubtitleFrame> m_frames;

};

extern SubtitleDecoderId SubtitleDecoderId_FFmpeg;
FACTORY_REGISTER(SubtitleDecoder, FFmpeg, "FFmpeg")

SubtitleDecoderFFmpeg::SubtitleDecoderFFmpeg()
    : codec_ctx(nullptr)
{
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
    // relative to packet pts, in ms
    frame.begin = pkt->pts + FORCE_DOUBLE(sub.start_display_time) / 1000;
    frame.end = pkt->pts + FORCE_DOUBLE(sub.end_display_time) / 1000;
    for (unsigned i = 0; i < sub.num_rects; i++) {
        switch (sub.rects[i]->type) {
        case SUBTITLE_ASS:
            //qDebug("ass frame: %s", sub.rects[i]->ass);
            frame.text.append(PlainText::fromAss(sub.rects[i]->ass));
            frame.text.append("\n");
            break;
        case SUBTITLE_TEXT:
            //qDebug("txt frame: %s", sub.rects[i]->text);
            frame.text.append(sub.rects[i]->text);
            frame.text.append("\n");
            break;
        case SUBTITLE_BITMAP:
            //sub.rects[i]->w > 0 && sub.rects[i]->h > 0
            //qDebug("bmp sub");
            frame = SubtitleFrame(); // not support bmp subtitle now
            break;
        default:
            break;
        }
    }
    avsubtitle_free(&sub);
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
        if (frame.isValid())
            m_frames.push_back(frame);
    }
    avcodec_close(codec_ctx);
    codec_ctx = nullptr;
    return true;
}

NAMESPACE_END