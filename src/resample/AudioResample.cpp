#include "AudioResample.h"
#include "AudioResample_p.h"
#include "mkid.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(AudioResample)

AudioResampleId AudioResampleId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
AudioResampleId AudioResampleId_SoundTouch = mkid::id32base36_6<'S', 'o', 'u', 'n', 'd', 'T'>::value;

AudioResample::AudioResample(AudioResamplePrivate *d):
    d_ptr(d)
{

}

AudioResample::~AudioResample()
{

}

const ByteArray& AudioResample::outData() const
{
    DPTR_D(const AudioResample);
    return d->data;
}

float AudioResample::speed() const
{
    DPTR_D(const AudioResample);
    return d->speed;
}

void AudioResample::setSpeed(float s)
{
    DPTR_D(AudioResample);
    d->speed = s;
}

void AudioResample::setInFormat(const AudioFormat &fmt)
{
    DPTR_D(AudioResample);
    d->in_format = fmt;
}

void AudioResample::setOutFormat(const AudioFormat &fmt)
{
    DPTR_D(AudioResample);
    if (d->out_format == fmt)
        return;
    d->out_format = fmt;
    prepare();
}

void AudioResample::setOutSampleFormat(int sample_fmt)
{
    DPTR_D(AudioResample);
    AudioFormat fmt(d->out_format);
    fmt.setSampleFormatFFmpeg(sample_fmt);
    setOutFormat(fmt);
}

void AudioResample::setInSamplesPerChannel(int spc)
{
    DPTR_D(AudioResample);
    d->in_samples_per_channel = spc;
}

int AudioResample::outSamplesPerChannel() const
{
    DPTR_D(const AudioResample);
    return d->out_samples_per_channel;
}

void AudioResample::setWantedSamples(int w)
{
    DPTR_D(AudioResample);
    d->wanted_nb_samples = w;
}

NAMESPACE_END
