#include "AudioOutput.h"
#include "AVOutput_p.h"
#include "AudioOutputBackend.h"
#include "AVLog.h"
#include "innermath.h"
extern "C" {
#include "libavutil/common.h"
}

#include <cassert>
#include <vector>

NAMESPACE_BEGIN

// chunk
static const int kBufferSamples = 4096;
static const int kBufferCount = 8; // may wait too long at the beginning (oal) if too large. if buffer count is too small, can not play for high sample rate audio.

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

template<typename T, typename C>
class ring_api {
public:
    ring_api() : m_0(0), m_1(0), m_s(0) {}
    void push_back(const T &t);
    void pop_front();
    T &front() { return m_data[m_0]; }
    const T &front() const { return m_data[m_0]; }
    T &back() { return m_data[m_1]; }
    const T &back() const { return m_data[m_1]; }
    virtual size_t capacity() const = 0;
    size_t size() const { return m_s; }
    bool empty() const { return size() == 0; }
    // need at() []?
    const T &at(size_t i) const { assert(i < m_s); return m_data[index(m_0 + i)]; }
    const T &operator[](size_t i) const { return at(i); }
    T &operator[](size_t i) { assert(i < m_s); return m_data[index(m_0 + i)]; }
protected:
    size_t index(size_t i) const { return i < capacity() ? i : i - capacity(); } // i always [0,capacity())
    size_t m_0, m_1;
    size_t m_s;
    C m_data;
};

template<typename T>
class ring : public ring_api<T, std::vector<T> > {
    using ring_api<T, std::vector<T> >::m_data; // why need this?
public:
    ring(size_t capacity = 16) : ring_api<T, std::vector<T> >() {
        m_data.reserve(capacity);
        m_data.resize(capacity);
    }
    size_t capacity() const { return m_data.size(); }
};
template<typename T, int N>
class static_ring : public ring_api<T, T[N]> {
    using ring_api<T, T[N]>::m_data; // why need this?
public:
    static_ring() : ring_api<T, T[N]>() {}
    size_t capacity() const { return N; }
};

template<typename T, typename C>
void ring_api<T, C>::push_back(const T &t) {
    if (m_s == capacity()) {
        m_data[m_0] = t;
        m_0 = index(++m_0);
        m_1 = index(++m_1);
    }
    else if (empty()) {
        m_s = 1;
        m_0 = m_1 = 0;
        m_data[m_0] = t;
    }
    else {
        m_data[index(m_0 + m_s)] = t;
        ++m_1;
        ++m_s;
    }
}

template<typename T, typename C>
void ring_api<T, C>::pop_front() {
    assert(!empty());
    if (empty())
        return;
    m_data[m_0] = T(); //erase the old data
    m_0 = index(++m_0);
    --m_s;
}

struct FrameInfo {
    FrameInfo(int s = 0, double t = 0, int64_t us = 0) : timestamp(t), duration(us), size(s) {}
    double timestamp;
    int64_t duration; // in us
    int size;
};

class AudioOutputPrivate: public AVOutputPrivate
{
public:
    AudioOutputPrivate() :
        backend(nullptr),
        available(false),
        buffer_samples(kBufferSamples),
        nb_buffers(kBufferCount),
        resample_type(ResampleBase),
        mute(false),
        support_mute(true),
        volume(1.0),
        volume_i(256),
        support_volume(true),
        buffers(kBufferCount),
        features(0),
        play_pos(0),
        processed_remain(0),
        msecs_ahead(0),
        paused(false)
    {
        
    }
    ~AudioOutputPrivate()
    {
        if (backend) {
            backend->close();
            delete backend;
            backend = nullptr;
        }
    }

    void tryVolume(float value);

    void updateScaleSamples()
    {
        scale_samples = get_scaler_func(format.sampleFormat(), volume, &volume_i);
    }

    void resetStatus() {
        play_pos = 0;
        processed_remain = 0;
        msecs_ahead = 0;
        frame_infos = ring<FrameInfo>(nb_buffers);
        cond.notify_all();
    }

    virtual void uwait(int64_t us) {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait_for(lock, std::chrono::microseconds((us + 500LL) / 1000LL));
    }

    AudioFormat format;
    AudioFormat requested;
    std::vector<std::string> backendNames;
    AudioOutputBackend *backend;
    bool available;
    int buffer_samples;
    int nb_buffers;
    ResampleType resample_type;
    bool mute, support_mute;
    float volume; uint16_t volume_i; bool support_volume;
    int buffers;
    int features;
    int play_pos; // index or bytes
    int processed_remain;
    int msecs_ahead;
    ring<FrameInfo> frame_infos;

    bool paused;
    scale_samples_func scale_samples;
};

void AudioOutputPrivate::tryVolume(float value)
{
    // if not open, try later
    if (!available)
        return;
    //if (features & AudioOutput::SetVolume) {
        support_volume = backend->setVolume(value);
        //if (!qFuzzyCompare(backend->volume(), value))
        //    sw_volume = true;
        if (support_volume)
            backend->setVolume(1.0); // TODO: partial software?
    //}
    //else {
    //    support_volume = true;
    //}
}

AudioOutput::AudioOutput():
    AVOutput(new AudioOutputPrivate)
{
    std::vector<std::string> backends = backendsAvailable();
    for (unsigned i = 0; i < backends.size(); ++i) {
        AVDebug("audio output backend: ", backends.at(i));
    }
    setBackend(AudioOutputBackend::defaultPriority());
}

AudioOutput::~AudioOutput()
{
    close();
}

std::vector<string> AudioOutput::backendsAvailable()
{
    static std::vector<string> backends;
    if (!backends.empty())
        return backends;
    if (AudioOutputBackend::count() <= 0)
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
    d->backend->setBufferSize(bufferSize());
    d->backend->setBufferCount(d->buffers);
    d->resetStatus();
    if (!d->backend->open())
        return false;
    d->available = true;
    d->tryVolume(volume());
    return true;
}

bool AudioOutput::isOpen() const
{
    return d_func()->available;
}

bool AudioOutput::close()
{
    DPTR_D(AudioOutput);
    d->resetStatus();
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
    if (d->paused)
        return false;
    if (!d->backend)
        return false;
    if (!receiveData(data, size, pts))
        return false;
    //d->backend->write(data, size);
    d->backend->play();
    return true;
}

bool AudioOutput::pause(bool flag)
{
    DPTR_D(AudioOutput);
    if (d->paused == flag)
        return true;
    d->paused = flag;
    if (d->backend)
        return d->backend->pause(flag);
    return false;
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

int AudioOutput::bufferCount() const
{
    DPTR_D(const AudioOutput);
    return d->nb_buffers;
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
    if (d->paused)
        return false;
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
    if (!waitForNextBuffer()) { // TODO: wait or not parameter, set by user (async)
        AVWarning("ao backend maybe not open\n");
        //d->resetStatus();
        return false;
    }
    // TODO: need check paused flag here? Now check flag in audiooutputdirectsound class
    //if (d->paused)
    //    return false;
    d->frame_infos.push_back(FrameInfo(size, pts, d->format.durationForBytes(static_cast<int64_t>(size))));
    return d->backend->write(queue_data.constData(), size);
}

bool AudioOutput::waitForNextBuffer()
{
    DPTR_D(AudioOutput);

    if (d->frame_infos.empty())
        return true;

    int remove = 0;
    const AudioOutputBackend::BufferControl f = d->backend->bufferControl();
    const FrameInfo &info = d->frame_infos.front();

    if (f & AudioOutputBackend::Blocking) {
        remove = 1;
    }
    else if (f & AudioOutputBackend::CountCallback) {
        d->backend->acquireNextBuffer();
        remove = 1;
    }
    else if (f & AudioOutputBackend::OffsetBytes) { //TODO: similar to Callback+getWritableBytes()
        int s = d->backend->getOffsetByBytes();
        int processed = s - d->play_pos;
        //qDebug("s: %d, play_pos: %d, processed: %d, bufferSizeTotal: %d", s, d.play_pos, processed, bufferSizeTotal());
        if (processed < 0)
            processed += bufferSize() * bufferCount();
        d->play_pos = s;
        const int next = info.size;
        int writable_size = d->processed_remain + processed;
        while ((writable_size < info.size) && next > 0) {
            const int64_t us = d->format.durationForBytes(next - writable_size);
            d->uwait(us);
            s = d->backend->getOffsetByBytes();
            processed += s - d->play_pos;
            if (processed < 0)
                processed += bufferSize() * bufferCount();
            writable_size = d->processed_remain + processed;
            d->play_pos = s;
        }
        d->processed_remain += processed;
        d->processed_remain -= info.size; //ensure d.processed_remain later is greater
        remove = -processed;
    }
    if (remove < 0) {
        int next = info.size;
        int free_bytes = -remove;
        while (free_bytes >= next && next > 0) {
            free_bytes -= next;
            if (d->frame_infos.empty()) {
                break;
            }
            d->frame_infos.pop_front();
            next = d->frame_infos.front().size;
        }
        //qDebug("remove: %d, unremoved bytes < %d, writable_bytes: %d", remove, free_bytes, d.processed_remain);
        return true;
    }
    //qDebug("remove count: %d", remove);
    while (remove-- > 0) {
        if (d->frame_infos.empty()) {
            break;
        }
        d->frame_infos.pop_front();
    }
    return true;
}

void AudioOutput::onCallback()
{
    DPTR_D(AudioOutput);
    d->cond.notify_all();
}

NAMESPACE_END
