#include "AudioResample.h"
#include "AudioResample_p.h"
#include "Factory.h"
#include "AVLog.h"
#include "inner.h"
#include "innermath.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libswresample/swresample.h"
#ifdef __cplusplus
}
#endif
NAMESPACE_BEGIN

class AudioResampleFFmpegPrivate: public AudioResamplePrivate
{
public:
    AudioResampleFFmpegPrivate():
        context(nullptr)
    {

    }
    ~AudioResampleFFmpegPrivate()
    {
        if (context) {
            swr_free(&context);
            context = nullptr;
        }
    }

    SwrContext *context;
};

class AudioResampleFFmpeg: public AudioResample
{
    DPTR_DECLARE_PRIVATE(AudioResampleFFmpeg)
public:
    AudioResampleFFmpeg();
    ~AudioResampleFFmpeg();

    virtual bool prepare();
    virtual bool convert(const uchar **data);
};

extern AudioResampleId AudioResampleId_FFmpeg;
FACTORY_REGISTER(AudioResample, FFmpeg, "FFmpeg")

AudioResampleFFmpeg::AudioResampleFFmpeg():
    AudioResample(new AudioResampleFFmpegPrivate)
{

}

AudioResampleFFmpeg::~AudioResampleFFmpeg()
{

}

bool AudioResampleFFmpeg::prepare()
{
    DPTR_D(AudioResampleFFmpeg);
    if (!d->in_format.isValid()) {
        AVWarning("in format is not valid.\n");
        return false;
    }
    if (!d->in_format.channels()) {
        if (!d->in_format.channelLayoutFFmpeg()) {
            d->in_format.setChannels(2);
            d->in_format.setChannelLayoutFFmpeg(av_get_default_channel_layout(2));
        } else {
            d->in_format.setChannels(av_get_channel_layout_nb_channels(d->in_format.channelLayoutFFmpeg()));
        }
    }
    if (!d->out_format.channels()) {
        if (!d->out_format.channelLayoutFFmpeg()) {
            d->out_format.setChannels(av_get_channel_layout_nb_channels(d->out_format.channelLayoutFFmpeg()));
        } else {
            d->out_format.setChannels(d->in_format.channels());
            d->out_format.setChannelLayoutFFmpeg(av_get_default_channel_layout(d->in_format.channels()));
        }
    }
    if (!d->out_format.sampleRate()) {
        d->out_format.setSampleRate(d->in_format.sampleRate());
    }
    if (d->context)
        swr_free(&d->context);
    d->context = swr_alloc_set_opts(d->context,
        d->out_format.channelLayoutFFmpeg(),
        (AVSampleFormat)d->out_format.sampleFormatFFmpeg(),
        d->out_format.sampleRate()/* / d->speed*/,
        d->in_format.channelLayoutFFmpeg(),
        (AVSampleFormat)d->in_format.sampleFormatFFmpeg(),
        d->in_format.sampleRate(),
        0, nullptr);

    if (!d->context) {
        AVWarning("swr alloc failed!\n");
        return false;
    }
    int ret = 0;
    ret = swr_init(d->context);
    if (ret < 0) {
        AVWarning("swr init failed: %s.\n", averror2str(ret));
        swr_free(&d->context);
        d->context = nullptr;
        return false;
    }
    return true;
}

bool AudioResampleFFmpeg::convert(const uchar **data)
{
    DPTR_D(AudioResampleFFmpeg);
    /**
     * function: int64_t swr_get_delay(struct SwrContext *s, int64_t base);
     * Here base is the sampling rate of input audio A, 44100 is the sampling rate of 
     * output video B. This line of code means that if 1024 audio A is converted, 
     * how many audio B can be generated, then why use swr_get_delay? In this way,
     * when our project is a real-time push project, suppose our input audio is base 
     * sampling rate, push output requires 44100 sampling rate, in order to ensure the 
     * synchronization of audio and video, we must ensure that every second Output 
     * 44100 * 2 sampling points B (here assumed to be dual-channel), but we need to 
     * know that the resampling itself also takes time, so we need to calculate this time
     * and convert it into several sampling points equivalent to the input audio A, In this way,
     * the output per second can be guaranteed to be 44100*2 sampling points B, 
     * thereby ensuring the synchronization of audio and video.
     */   
    int in_sample_rate = d->in_format.sampleRate();
    int out_sample_rate = d->out_format.sampleRate();
    int64_t delay = swr_get_delay(d->context, std::max(in_sample_rate, out_sample_rate));
 
    //Use for "a*b/c"
    d->out_samples_per_channel = av_rescale_rnd(
        delay + (int64_t)d->in_samples_per_channel,
        out_sample_rate,
        (int64_t)in_sample_rate,
        AV_ROUND_UP);
#if 1
    d->data.reset();
    int64_t out_count = 0;
    int out_size = 0;
    /**
     * I think it's better to changed wanted_nb_samples when speed changed, 
     * rather than to change sample-rate
     */
    int wanted_nb_samples = d->wanted_nb_samples / d->speed;
    if (wanted_nb_samples != d->in_samples_per_channel) {
        out_count = (int64_t)wanted_nb_samples * out_sample_rate / in_sample_rate + 256;
        out_size = av_samples_get_buffer_size(NULL,
            d->out_format.channels(),
            out_count,
            (AVSampleFormat)d->out_format.sampleFormatFFmpeg(),
            0);
        if (out_size < 0) {
            AVDebug("av_samples_get_buffer_size() failed\n");
            return false;
        }
        if (swr_set_compensation(d->context, 
            (wanted_nb_samples - d->in_samples_per_channel) * out_sample_rate / in_sample_rate,
            wanted_nb_samples * out_sample_rate / in_sample_rate) < 0) {
            AVDebug("swr_set_compensation() failed\n");
            return false;
        }
    } else {
        int sampleSize = d->out_format.channels() * d->out_format.bytesPerSample();
        out_size = d->out_samples_per_channel * sampleSize;
        out_count = d->out_samples_per_channel;
    }
    if (out_size > d->data.size()) {
        d->data.resize(out_size);
    }
    uint8_t *out = (uint8_t*)d->data.data();
    int samples = swr_convert(d->context, &out, out_count,
        data, d->in_samples_per_channel);
    d->out_samples_per_channel = samples;
    int resample_size = d->out_samples_per_channel *  d->out_format.channels() * d->out_format.bytesPerSample();
    d->data.resize(resample_size);
    //AVDebug("wanted_nb_samples: %d\n", d->wanted_nb_samples);
#else
    int sampleSize = d->out_format.channels() * d->out_format.bytesPerSample();
    int64_t outSize = d->out_samples_per_channel * sampleSize;
    if (outSize > d->data.size()) {
        d->data.resize(outSize);
    }
    uint8_t *out = (uint8_t*)d->data.data();
    int ospc = swr_convert(d->context, &out, d->out_samples_per_channel,
                                          data, d->in_samples_per_channel);
    d->out_samples_per_channel = ospc;
    /**
     * It is important to resize data.
     * Otherwise, audio 'dudu' will appear.
     */
    int resample_size = d->out_samples_per_channel * sampleSize;
    d->data.resize(resample_size);
#endif
    return true;
}

NAMESPACE_END
