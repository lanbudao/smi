#include "AudioOutputBackend.h"
#include "AudioOutputBackend_p.h"
#include "Factory.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(AudioOutputBackend)

AudioOutputBackend::AudioOutputBackend(AudioOutputBackendPrivate* p):
    d_ptr(p)
{

}

AudioOutputBackend::~AudioOutputBackend()
{

}

std::vector<std::string> AudioOutputBackend::defaultPriority()
{
    static std::vector<std::string> backends;

#ifdef FFPROC_HAVE_DSOUND
    backends.push_back("Dsound");
#endif // FFPROC_HAVE_DSOUND
#ifdef FFPROC_HAVE_PORTAUDIO
    backends.push_back("PortAudio");
#endif

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

void AudioOutputBackend::setBufferSize(int size)
{
    d_func()->buffer_size = size;
}

void AudioOutputBackend::setBufferCount(int count)
{
    d_func()->buffer_count = count;
}

const AudioFormat *AudioOutputBackend::format()
{
    DPTR_D(const AudioOutputBackend);
    return &d->format;
}

NAMESPACE_END
