#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include "AVOutput.h"
#include "AudioFormat.h"
#include <vector>

NAMESPACE_BEGIN

class AudioOutputPrivate;
class AudioOutput: public AVOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutput)
public:
    AudioOutput();
    ~AudioOutput();

    static std::vector<std::string> backendsAvailable();

    bool open();
    bool isOpen() const;
    bool close();
    bool write(const char *data, int size, double pts);

    AudioFormat setAudioFormat(const AudioFormat& format);
    AudioFormat audioFormat() const;
    void setResampleType(ResampleType t);

    void setBackend(const std::vector<std::string> &names);

    int bufferSize() const;

    int bufferSamples() const;
    void setBufferSamples(int value);

    float volume() const;
    void setVolume(float v);

    bool isMute() const;
    void setMute(bool m);

    bool receiveData(const char* data, int size, double pts);

private:

};

NAMESPACE_END
#endif //AUDIO_OUTPUT_H
