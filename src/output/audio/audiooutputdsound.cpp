#include "sdk/global.h"
#include "AudioOutputBackend.h"

NAMESPACE_BEGIN

class AudioOutputDSound PU_NO_COPY: public AudioOutputBackend
{
public:
    AudioOutputDSound();
    ~AudioOutputDSound() PU_DECL_OVERRIDE;

    bool initialize();
    bool uninitialize();


    bool open() PU_DECL_OVERRIDE;
    bool close() PU_DECL_OVERRIDE;
    bool write(const char *data, int size) PU_DECL_OVERRIDE;

private:


    bool isInitialized;
    double outputLatency;
};

AudioOutputDSound::AudioOutputDSound()
{

}

AudioOutputDSound::~AudioOutputDSound()
{

}

bool AudioOutputDSound::initialize()
{

    return true;
}

bool AudioOutputDSound::uninitialize()
{

    return true;
}

bool AudioOutputDSound::open()
{
    return true;
}

bool AudioOutputDSound::close()
{
    return true;
}

bool AudioOutputDSound::write(const char *data, int size)
{
    return true;
}



NAMESPACE_END
