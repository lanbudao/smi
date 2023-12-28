#include "AudioDecoder.h"
#include "AVDecoder_p.h"
#include "Factory.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(AudioDecoder)


AudioDecoder::AudioDecoder(AudioDecoderPrivate *d):
    AVDecoder(d)
{

}

AudioDecoder::~AudioDecoder()
{

}

void AudioDecoder::onOpen()
{
    DPTR_D(AudioDecoder);
    if ((d->fmt_ctx->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK))
        && !d->fmt_ctx->iformat->read_seek) {
        d->start_pts = d->current_stream->start_time;
        d->start_pts_tb = d->current_stream->time_base;
    }
}

StringList AudioDecoder::supportedCodecs()
{
    static StringList codecs;
    if (!codecs.empty())
        return codecs;
    avcodec_register_all();
    AVCodec* c = nullptr;
    while ((c=av_codec_next(c))) {
        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_AUDIO)
            continue;
        codecs.push_back(c->name);
    }
    return codecs;
}

std::string AudioDecoder::name() const {
    return std::string();
}

std::string AudioDecoder::description() const {
    return std::string();
}

void AudioDecoder::setAudioResample(AudioResample *resample)
{
    DPTR_D(AudioDecoder);
    d->resample = resample;
}

AudioResample *AudioDecoder::audioResample() const
{
    DPTR_D(const AudioDecoder);
    return d->resample;
}

void AudioDecoder::setResampleType(ResampleType t)
{
    DPTR_D(AudioDecoder);
    d->resample_type = t;
    if (t == ResampleBase) {
        d->resample = AudioResample::create(AudioResampleId_FFmpeg);
        if (d->resample) {
            d->resample->setOutSampleFormat(AV_SAMPLE_FMT_FLT);
        }
    }
    else {
        d->resample = AudioResample::create(AudioResampleId_SoundTouch);
        if (d->resample) {
            d->resample->setOutSampleFormat(AV_SAMPLE_FMT_S16);
        }
    }
}

NAMESPACE_END
