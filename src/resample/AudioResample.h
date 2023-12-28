#ifndef AUDIO_RESAMPLE_H
#define AUDIO_RESAMPLE_H

#include "ByteArray.h"
#include "Factory.h"

typedef int AudioResampleId;

NAMESPACE_BEGIN

class AudioFormat;
class AudioResamplePrivate;
class AudioResample
{
    FACTORY_INTERFACE(AudioResample)
    DPTR_DECLARE_PRIVATE(AudioResample)
public:
    virtual ~AudioResample();

    const ByteArray& outData() const;

    float speed() const;
    void setSpeed(float s);
    virtual bool prepare() = 0;
    virtual bool convert(const uchar **data) = 0;

    void setInFormat(const AudioFormat &fmt);
    void setOutFormat(const AudioFormat &fmt);
    void setOutSampleFormat(int sample_fmt);
    void setInSamplesPerChannel(int spc);

    int outSamplesPerChannel() const;
    void setWantedSamples(int w);

protected:
    AudioResample(AudioResamplePrivate *d);
    DPTR_DECLARE(AudioResample)
};

extern AudioResampleId AudioResampleId_FFmpeg;
extern AudioResampleId AudioResampleId_SoundTouch;

NAMESPACE_END
#endif //AUDIO_RESAMPLE_H
