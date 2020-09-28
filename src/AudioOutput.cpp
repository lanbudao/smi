#include "AudioOutput.h"
#include "AVOutput_p.h"
#include "AudioOutputBackend.h"
#include "AVLog.h"

NAMESPACE_BEGIN

static const int kBufferSamples = 512;
static const int kBufferCount = 8*2;

class AudioOutputPrivate: public AVOutputPrivate
{
public:
    AudioOutputPrivate():
        backend(nullptr),
        available(false),
        buffer_samples(kBufferSamples)
    {

    }
    ~AudioOutputPrivate()
    {

    }

    AudioFormat format;
    AudioFormat requested;
    std::vector<std::string> backendNames;
    AudioOutputBackend *backend;
    bool available;
    int buffer_samples;
};

AudioOutput::AudioOutput():
    AVOutput(new AudioOutputPrivate)
{
    std::vector<std::string> backends = backendsAvailable();
    for (int i = 0; i < backends.size(); ++i) {
        AVDebug("audio output backend: ", backends.at(i));
    }
    setBackend(AudioOutputBackend::defaultPriority());
}

AudioOutput::~AudioOutput()
{

}

extern void AudioOutput_RegisterAll();
std::vector<string> AudioOutput::backendsAvailable()
{
    AudioOutput_RegisterAll();
    static std::vector<string> backends;
    if (!backends.empty())
        return backends;
    AudioOutputBackendId *i = nullptr;
    while ((i = AudioOutputBackend::next(i)) != nullptr) {
        backends.push_back(AudioOutputBackend::name(*i));
    }
    return backends;
}

bool AudioOutput::open()
{
    DPTR_D(AudioOutput);
    if (!d->backend)
        return false;
    d->backend->setFormat(d->format);
    if (!d->backend->open())
        return false;
    d->available = true;
    return true;
}

bool AudioOutput::isOpen() const
{
    return d_func()->available;
}

bool AudioOutput::close()
{
    DPTR_D(AudioOutput);
	if (!d->available)
		return true;
    if (!d->backend)
        return false;
    if (!d->backend->close())
        return false;
    d->available = false;
    return true;
}

bool AudioOutput::write(const char *data, int size, double pts)
{
    DPTR_D(AudioOutput);
    if (!d->backend)
        return false;
    d->backend->write(data, size);
    return true;
}

AudioFormat AudioOutput::setAudioFormat(const AudioFormat &format)
{
    DPTR_D(AudioOutput);
    if (d->format == format)
        return format;
    d->requested = format;
    if (!d->backend) {
        d->format = AudioFormat();
//        d->scale_samples = NULL;
        return AudioFormat();
    }
    if (d->backend->isSupported(format)) {
        d->format = format;
//        d->updateSampleScaleFunc();
        return format;
    }
    AudioFormat af(format);
    // set channel layout first so that isSupported(AudioFormat) will not always false
    if (!d->backend->isSupported(format.channelLayout()))
        af.setChannelLayout(AudioFormat::ChannelLayout_Stereo); // assume stereo is supported
    bool check_up = af.bytesPerSample() == 1;
    while (!d->backend->isSupported(af) && !d->backend->isSupported(af.sampleFormat())) {
        if (af.isPlanar()) {
            af.setSampleFormat(ToPacked(af.sampleFormat()));
            continue;
        }
        if (af.isFloat()) {
            if (af.bytesPerSample() == 8)
                af.setSampleFormat(AudioFormat::SampleFormat_Float);
            else
                af.setSampleFormat(AudioFormat::SampleFormat_Signed32);
        } else {
            af.setSampleFormat(AudioFormat::make(af.bytesPerSample()/2, false, (af.bytesPerSample() == 2) | af.isUnsigned() /* U8, no S8 */, false));
        }
        if (af.bytesPerSample() < 1) {
            if (!check_up) {
                AVWarning("No sample format found!\n");
                break;
            }
            af.setSampleFormat(AudioFormat::SampleFormat_Float);
            check_up = false;
            continue;
        }
    }
    d->format = af;
//    d->updateSampleScaleFunc();
    return af;
}

AudioFormat AudioOutput::audioFormat() const
{
    DPTR_D(const AudioOutput);
    return d->format;
}

void AudioOutput::setBackend(const std::vector<std::string> &names)
{
    DPTR_D(AudioOutput);
    d->backendNames = names;

    if (d->backend) {
        d->backend->close();
        delete d->backend;
        d->backend = nullptr;
    }
    int size = d->backendNames.size();
    for (int i = 0; i < size; ++i) {
        d->backend = AudioOutputBackend::create(d->backendNames.at(i).c_str());
        if (!d->backend)
            continue;
        if (d->backend->avaliable())
            break;
        delete d->backend;
        d->backend = nullptr;
    }
    if (d->backend) {

    }
}

int AudioOutput::bufferSize() const
{
    DPTR_D(const AudioOutput);
    return d->buffer_samples * d->format.bytesPerSample();
}

int AudioOutput::bufferSamples() const
{
    DPTR_D(const AudioOutput);
    return d->buffer_samples;
}

void AudioOutput::setBufferSamples(int value)
{
    DPTR_D(AudioOutput);
    d->buffer_samples = value;
}

NAMESPACE_END
