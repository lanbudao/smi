#include "AudioOutputBackend.h"
#include "AudioOutputBackend_p.h"
#include "Factory.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(AudioOutputBackend)

void AudioOutput_RegisterAll()
{
    ONLY_RUN_ONES;
    if (!AudioOutputBackendFactory::instance().registeredIds().empty()) {
        return;
    }
    extern bool RegisterAudioOutputBackendPortAudio_Man();
    RegisterAudioOutputBackendPortAudio_Man();
}

AudioOutputBackend::AudioOutputBackend():
    d_ptr(new AudioOutputBackendPrivate)
{

}

AudioOutputBackend::~AudioOutputBackend()
{

}

std::vector<std::string> AudioOutputBackend::defaultPriority()
{
    static std::vector<std::string> backends;

    backends.push_back("PortAudio");

    return backends;
}

bool AudioOutputBackend::avaliable() const
{
    DPTR_D(const AudioOutputBackend);
    return d->avaliable;
}

void AudioOutputBackend::setAvaliable(bool b)
{
    DPTR_D(AudioOutputBackend);
    d->avaliable = b;
}

void AudioOutputBackend::setFormat(const AudioFormat &fmt)
{
    DPTR_D(AudioOutputBackend);
    d->format = fmt;
}

const AudioFormat *AudioOutputBackend::format()
{
    DPTR_D(const AudioOutputBackend);
    return &d->format;
}

NAMESPACE_END
