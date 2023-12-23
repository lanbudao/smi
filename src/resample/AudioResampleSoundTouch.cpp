#include "AudioResample.h"
#include "AudioResample_p.h"
#include "Factory.h"
#include "AVLog.h"
#include "inner.h"
#include "innermath.h"

//#define SOUNDTOUCH_FLOAT_SAMPLES 1
//#define SOUNDTOUCH_INTEGER_SAMPLES 1
#include "SoundTouch.h"
using namespace soundtouch;

extern "C" {
#include "libswresample/swresample.h"
}
NAMESPACE_BEGIN

using namespace soundtouch;
class AudioResampleSoundTouchPrivate: public AudioResamplePrivate
{
public:
    AudioResampleSoundTouchPrivate():
        context(nullptr)
    {

    }
    ~AudioResampleSoundTouchPrivate()
    {
        if (context) {
            swr_free(&context);
            context = nullptr;
        }
        if (audio_buf)
            av_free(audio_buf);
        if (audio_new_buf)
            av_free(audio_new_buf);
    }

    int translate(SAMPLETYPE *data, int len, int bytes_per_sample,
            int channel, int sampleRate);

    SwrContext *context;
    SoundTouch touch;
    uint8_t* audio_buf = nullptr;
    unsigned int audio_buf_size = 0;
    SAMPLETYPE* audio_new_buf = nullptr;
    unsigned int audio_new_buf_size = 0;
    int translate_time = 1;
};

class AudioResampleSoundTouch: public AudioResample
{
    DPTR_DECLARE_PRIVATE(AudioResampleSoundTouch)
    public:
        AudioResampleSoundTouch();
    ~AudioResampleSoundTouch();

    virtual bool prepare();
    virtual bool convert(const uchar **data);
};

extern AudioResampleId AudioResampleId_SoundTouch;
FACTORY_REGISTER(AudioResample, SoundTouch, "SoundTouch")

AudioResampleSoundTouch::AudioResampleSoundTouch():
    AudioResample(new AudioResampleSoundTouchPrivate)
{

}

AudioResampleSoundTouch::~AudioResampleSoundTouch()
{

}

bool AudioResampleSoundTouch::prepare()
{
    DPTR_D(AudioResampleSoundTouch);
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

bool AudioResampleSoundTouch::convert(const uchar **data)
{
    DPTR_D(AudioResampleSoundTouch);

    int in_sample_rate = d->in_format.sampleRate();
    int out_sample_rate = d->out_format.sampleRate()/* / d->speed*/;

    d->data.reset();
    int wanted_nb_samples = d->wanted_nb_samples/* / d->speed*/;
    // why plus 256? make sure the audio_buf is enough.
    int out_count = (int64_t)wanted_nb_samples * out_sample_rate / in_sample_rate + 256;
    int out_size = av_samples_get_buffer_size(NULL,
                                          d->out_format.channels(),
                                          out_count,
                                          (AVSampleFormat)d->out_format.sampleFormatFFmpeg(),
                                          0);
    if (out_size < 0) {
        AVDebug("av_samples_get_buffer_size() failed\n");
        return false;
    }
    if (wanted_nb_samples != d->in_samples_per_channel) {
        if (swr_set_compensation(d->context,
                                 (wanted_nb_samples - d->in_samples_per_channel) * out_sample_rate / in_sample_rate,
                                 wanted_nb_samples * out_sample_rate / in_sample_rate) < 0) {
            AVDebug("swr_set_compensation() failed\n");
            return false;
        }
    }
    av_fast_malloc(&d->audio_buf, &d->audio_buf_size, out_size);
    int len = swr_convert(d->context, &d->audio_buf, out_count,
                           data, d->in_samples_per_channel);
    d->out_samples_per_channel = len;
    int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)d->out_format.sampleFormatFFmpeg());
    int resampled_data_size = len * d->out_format.channels() * bytes_per_sample;

    if (FuzzyCompare(d->speed, 1.0f)) {
        d->data.setData((const char*)d->audio_buf, resampled_data_size);
        return true;
    }
    // soundtouch proc
    av_fast_malloc(&d->audio_new_buf, &d->audio_new_buf_size, out_size * d->translate_time);
    // translate uint8_t to short
#if SOUNDTOUCH_INTEGER_SAMPLES
    for (int i = 0; i < (resampled_data_size / 2); i++) {
        d->audio_new_buf[i] = (d->audio_buf[i * 2] | (d->audio_buf[i * 2 + 1] << 8));
    }
#else
    for (int i = 0; i < (resampled_data_size / 4); i++) {
        d->audio_new_buf[i] = (d->audio_buf[i * 4] |
                (d->audio_buf[i * 4 + 1] << 8) |
                (d->audio_buf[i * 4 + 2] << 16) |
                (d->audio_buf[i * 4 + 3] << 24));
    }
#endif
    int ret_len = d->translate(d->audio_new_buf, resampled_data_size / bytes_per_sample, bytes_per_sample,
                                           d->out_format.channels(),
                                           d->out_format.sampleRate());
    if (ret_len > 0) {
        resampled_data_size = ret_len;
        d->data.setData((const char*)d->audio_new_buf, resampled_data_size);
        d->translate_time = 1;
        return true;
    }
    d->translate_time++;
    return false;
}

int AudioResampleSoundTouchPrivate::translate(
        SAMPLETYPE *data, int len, int bytes_per_sample,
        int channel, int sampleRate)
{
    int put_n_sample = len / channel;
    int nb = 0;
    int pcm_data_size = 0;

    touch.setTempo(speed);

    touch.setSampleRate(sampleRate);
    touch.setChannels(channel);

    touch.putSamples((SAMPLETYPE*)data, put_n_sample);

    do {
        nb = touch.receiveSamples((SAMPLETYPE*)data, sampleRate / channel);
        pcm_data_size += nb * channel * bytes_per_sample;
    } while (nb != 0);

    return pcm_data_size;
}

NAMESPACE_END
