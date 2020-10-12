#include "AudioOutput.h"
#include "AVOutput_p.h"
#include "AudioOutputBackend.h"
#include "AVLog.h"
#include "innermath.h"
extern "C" {
#include "libavutil/common.h"
}

NAMESPACE_BEGIN

// chunk
static const int kBufferSamples = 512;
static const int kBufferCount = 8 * 2; // may wait too long at the beginning (oal) if too large. if buffer count is too small, can not play for high sample rate audio.

typedef void(*scale_samples_func)(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float volumef);

/// from libavfilter/af_volume begin
static inline void scale_samples_u8(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8(((((uint64_t)src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_u8_small(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8((((src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_s16(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16(((uint64_t)smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s16_small(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16((smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s32(uint8_t *dst, const uint8_t *src, int nb_samples, int volume, float)
{
    int32_t *smp_dst = (int32_t *)dst;
    const int32_t *smp_src = (const int32_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clipl_int32((((uint64_t)smp_src[i] * volume + 128) >> 8));
}
/// from libavfilter/af_volume end

//TODO: simd
template<typename T>
static inline void scale_samples(uint8_t *dst, const uint8_t *src, int nb_samples, int, float volume)
{
    T *smp_dst = (T *)dst;
    const T *smp_src = (const T *)src;
    for (int i = 0; i < nb_samples; ++i)
        smp_dst[i] = smp_src[i] * (T)volume;
}

scale_samples_func get_scaler_func(AudioFormat::SampleFormat fmt, float vol, uint16_t* voli)
{
    uint16_t v = (uint16_t)(vol * 256.0 + 0.5);
    if (voli)
        *voli = v;
    switch (fmt) {
    case AudioFormat::SampleFormat_Unsigned8:
    case AudioFormat::SampleFormat_Unsigned8Planar:
        return v < 0x1000000 ? scale_samples_u8_small : scale_samples_u8;
    case AudioFormat::SampleFormat_Signed16:
    case AudioFormat::SampleFormat_Signed16Planar:
        return v < 0x10000 ? scale_samples_s16_small : scale_samples_s16;
    case AudioFormat::SampleFormat_Signed32:
    case AudioFormat::SampleFormat_Signed32Planar:
        return scale_samples_s32;
    case AudioFormat::SampleFormat_Float:
    case AudioFormat::SampleFormat_FloatPlanar:
        return scale_samples<float>;
    case AudioFormat::SampleFormat_Double:
    case AudioFormat::SampleFormat_DoublePlanar:
        return scale_samples<double>;
    default:
        return 0;
    }
}

class AudioOutputPrivate: public AVOutputPrivate
{
public:
    AudioOutputPrivate() :
        backend(nullptr),
        available(false),
        buffer_samples(kBufferSamples),
        resample_type(ResampleBase),
        mute(false),
        support_mute(true),
        volume(1.0),
        volume_i(256),
        support_volume(true)
    {

    }
    ~AudioOutputPrivate()
    {

    }

    void updateScaleSamples()
    {
        scale_samples = get_scaler_func(format.sampleFormat(), volume, &volume_i);
    }

    AudioFormat format;
    AudioFormat requested;
    std::vector<std::string> backendNames;
    AudioOutputBackend *backend;
    bool available;
    int buffer_samples;
    ResampleType resample_type;
    bool mute, support_mute;
    float volume; uint16_t volume_i; bool support_volume;

    scale_samples_func scale_samples;
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
    if (!receiveData(data, size, pts))
        return false;
    //d->backend->write(data, size);
    d->backend->play();
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
        d->scale_samples = NULL;
        return AudioFormat();
    }
    if (d->backend->isSupported(format)) {
        d->format = format;
        d->updateScaleSamples();
        return format;
    }
    AudioFormat af(format);
    // set channel layout first so that isSupported(AudioFormat) will not always false
    if (!d->backend->isSupported(format.channelLayout()))
        af.setChannelLayout(AudioFormat::ChannelLayout_Stereo); // assume stereo is supported
    bool check_up = af.bytesPerSample() == 1;
    while (!d->backend->isSupported(af) && !d->backend->isSupported(af.sampleFormat())) {
        if (d->resample_type == ResampleSoundtouch) {
            af.setSampleFormat(AudioFormat::SampleFormat_Signed16);
            continue;
        }
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
    d->updateScaleSamples();
    return af;
}

AudioFormat AudioOutput::audioFormat() const
{
    DPTR_D(const AudioOutput);
    return d->format;
}

void AudioOutput::setResampleType(ResampleType t)
{
    DPTR_D(AudioOutput);
    d->resample_type = t;
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

float AudioOutput::volume() const
{
    return d_func()->volume;
}

void AudioOutput::setVolume(float v)
{
    DPTR_D(AudioOutput);
    d->volume = av_clipf_c(v, 0.0, 1.0);
    d->updateScaleSamples();
}

bool AudioOutput::isMute() const
{
    return d_func()->mute;
}

void AudioOutput::setMute(bool m)
{
    d_func()->mute = m;
}

bool AudioOutput::receiveData(const char* data, int size, double pts)
{
    DPTR_D(AudioOutput);
    ByteArray queue_data(data, size);
    if (isMute() && d->support_mute) {
        char s = 0;
        if (d->format.isUnsigned() && !d->format.isFloat())
            s = 1 << ((d->format.bytesPerSample() << 3) - 1);
        queue_data.fill(s);
    }
    else {
        if (!FuzzyCompare(volume(), 1.0f) && d->support_volume && d->scale_samples
            ) {
            // TODO: af_volume needs samples_align to get nb_samples
            const int nb_samples = queue_data.size() / d->format.bytesPerSample();
            uint8_t *dst = (uint8_t*)queue_data.constData();
            d->scale_samples(dst, dst, nb_samples, d->volume_i, volume());
        }
    }
    return d->backend->write(queue_data.constData(), size);
}

NAMESPACE_END
