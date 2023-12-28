#ifndef AUDIO_RESAMPLE_P_H
#define AUDIO_RESAMPLE_P_H

#include "AudioFormat.h"

NAMESPACE_BEGIN

class AudioResamplePrivate
{
public:
    AudioResamplePrivate():
        in_samples_per_channel(0),
        out_samples_per_channel(0),
        wanted_nb_samples(0),
        pitch(1.0),
        speed(1.0)
    {
        in_format.setSampleFormat(AudioFormat::SampleFormat_Unknown);
        out_format.setSampleFormat(AudioFormat::SampleFormat_Float);
    }
    virtual ~AudioResamplePrivate()
    {
    }

    int in_samples_per_channel, out_samples_per_channel;
    AudioFormat in_format, out_format;
    int wanted_nb_samples;

    ByteArray data;
    float pitch;
    float speed;
};

NAMESPACE_END
#endif //AUDIO_RESAMPLE_P_H
