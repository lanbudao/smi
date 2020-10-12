#ifndef AUDIOFRAME_H
#define AUDIOFRAME_H

#include "Frame.h"
#include "AudioFormat.h"

NAMESPACE_BEGIN

class AudioResample;
class AudioFramePrivate;
class AudioFrame: public Frame
{
    DPTR_DECLARE_PRIVATE(AudioFrame)
public:
    AudioFrame(const AudioFormat &format = AudioFormat(), const ByteArray& data = ByteArray());
    virtual ~AudioFrame();

    bool isValid() const;
    void setData(void* data);

    int channelCount() const;

    int64_t duration();

    AudioFormat format() const;

    int samplePerChannel() const;
    void setSamplePerChannel(int channel);

    void setAudioResampler(AudioResample *resample);
    AudioFrame to(const AudioFormat &fmt) const;

    int dataSize() const;
};

NAMESPACE_END
#endif //AUDIOFRAME_H
