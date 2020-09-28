#include "AVDecoder.h"
#include "AVDecoder_p.h"
#include "inner.h"

#include <sstream>

NAMESPACE_BEGIN

AVDecoder::AVDecoder(AVDecoderPrivate *d):
    d_ptr(d)
{
//    avcodec_register_all();
}

AVDecoder::~AVDecoder()
{

}

void AVDecoder::initialize(AVFormatContext *ctx, AVStream* stream)
{
    DPTR_D(AVDecoder);
    d->fmt_ctx = ctx;
    d->current_stream = stream;
}

AVCodecContext * AVDecoder::codecCtx()
{
    DPTR_D(AVDecoder);
    return d->codec_ctx;
}

bool AVDecoder::open(const string &extra)
{
    DPTR_D(AVDecoder);
    int ret;
    AVCodec *codec;
    PU_UNUSED(extra)

    d->codec_ctx = avcodec_alloc_context3(nullptr);
    if (!d->codec_ctx)
        return false;
    // Copy stream paras to codec context
    ret = avcodec_parameters_to_context(d->codec_ctx, d->current_stream->codecpar);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_WARNING, "No codec could be inited\n");
        avcodec_free_context(&d->codec_ctx);
        return false;
    }

    d->opened = false;
    if (!d->codec_ctx) {
        return false;
    }
    codec = findCodec(d->codec_name, d->hwaccel, d->codec_ctx->codec_id);
    if (!codec) {
        //TODO error signal
        return false;
    }
    if (!d->open()) {
        d->close();
        return false;
    }
    d->opened = true;
    AV_RUN_CHECK(avcodec_open2(d->codec_ctx, codec, &d->dict), false);
    return true;
}

bool AVDecoder::close()
{
    DPTR_D(AVDecoder);
    if (!d->opened)
        return true;
    flush();
    d->close();
    if (d->codec_ctx)
        AV_RUN_CHECK(avcodec_close(d->codec_ctx), false);
    return true;
}

bool AVDecoder::isOpen()
{
    DPTR_D(const AVDecoder);
    return d->opened;
}

void AVDecoder::flush()
{
    DPTR_D(AVDecoder);
    d->flush();
}

void AVDecoder::setCodecName(const std::string &name)
{
    DPTR_D(AVDecoder);

    if (d->codec_name == name)
        return;
    d->codec_name = name;
}

std::string AVDecoder::codecName() const
{
    DPTR_D(const AVDecoder);
    return d->codec_name;
}

void AVDecoderPrivate::flush()
{
    if (!opened)
        return;
    if (codec_ctx)
        avcodec_flush_buffers(codec_ctx);
}

void AVDecoderPrivate::applyOptionsForDict()
{
    //TODO
}

bool AVDecoder::isAvailable() const {
    DPTR_D(const AVDecoder);
    return d->codec_ctx != nullptr;
}

//void AVDecoder::setCodecCtx(void *ctx)
//{
//    DPTR_D(AVDecoder);
//    AVCodecContext *codec_ctx = static_cast<AVCodecContext *>(ctx);
//    if (codec_ctx == d->codec_ctx)
//        return;
//    if (isOpen())
//        close();
//    if (!codec_ctx) {
//        avcodec_free_context(&d->codec_ctx);
//        d->codec_ctx = nullptr;
//        return;
//    }
//    if (!d->codec_ctx)
//        d->codec_ctx = avcodec_alloc_context3(codec_ctx->codec);
//    if (!d->codec_ctx)
//        return;
//    AV_ENSURE_OK(avcodec_copy_context(d->codec_ctx, codec_ctx));
//}

AVCodec *AVDecoder::findCodec(const std::string &name, const std::string &hwaccel, int id)
{
    std::string fullName(name);
    if (fullName.empty()) {
        if (hwaccel.empty())
            return avcodec_find_decoder(static_cast<AVCodecID>(id));
        std::stringstream temp;
        temp << avcodec_get_name(static_cast<AVCodecID>(id)) << "_" << hwaccel;
        fullName = temp.str();
    }
    AVCodec *codec = avcodec_find_decoder_by_name(fullName.c_str());
    if (!codec)
        return nullptr;
    const AVCodecDescriptor *des = avcodec_descriptor_get_by_name(fullName.c_str());
    if (des)
        return avcodec_find_decoder(static_cast<AVCodecID>(des->id));
    return nullptr;
}

NAMESPACE_END
