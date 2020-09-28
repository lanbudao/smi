#include "AudioFormat.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN

static const struct {
    int64_t ff;
    AudioFormat::ChannelLayout cl;
} channelLayoutMap[] = {
        { AV_CH_FRONT_LEFT, AudioFormat::ChannelLayout_Left },
        { AV_CH_FRONT_RIGHT, AudioFormat::ChannelLayout_Right },
        { AV_CH_FRONT_CENTER, AudioFormat::ChannelLayout_Center },
        { AV_CH_LAYOUT_MONO, AudioFormat::ChannelLayout_Mono },
        { AV_CH_LAYOUT_STEREO, AudioFormat::ChannelLayout_Stereo },
        { 0, AudioFormat::ChannelLayout_Unsupported}
};

typedef struct {
    AVSampleFormat avfmt;
    AudioFormat::SampleFormat fmt;
    const char* name;
} sample_fmt_entry;
static const sample_fmt_entry samplefmts[] = {
    { AV_SAMPLE_FMT_U8, AudioFormat::SampleFormat_Unsigned8, "u8" },
    { AV_SAMPLE_FMT_S16, AudioFormat::SampleFormat_Signed16, "s16" },
    { AV_SAMPLE_FMT_S32, AudioFormat::SampleFormat_Signed32, "s32" },
    { AV_SAMPLE_FMT_FLT, AudioFormat::SampleFormat_Float, "float" },
    { AV_SAMPLE_FMT_DBL, AudioFormat::SampleFormat_Double, "double" },
    { AV_SAMPLE_FMT_U8P, AudioFormat::SampleFormat_Unsigned8Planar, "u8p" },
    { AV_SAMPLE_FMT_S16P, AudioFormat::SampleFormat_Signed16Planar, "s16p" },
    { AV_SAMPLE_FMT_S32P, AudioFormat::SampleFormat_Signed32Planar, "s32p" },
    { AV_SAMPLE_FMT_FLTP, AudioFormat::SampleFormat_FloatPlanar, "floatp" },
    { AV_SAMPLE_FMT_DBLP, AudioFormat::SampleFormat_DoublePlanar, "doublep" },
    { AV_SAMPLE_FMT_NONE, AudioFormat::SampleFormat_Unknown, "unknown" }
};

class AudioFormatPrivate
{
public:
    AudioFormatPrivate():
        sample_fmt(AudioFormat::SampleFormat_Unknown),
        av_sample_fmt(AV_SAMPLE_FMT_NONE),
        sample_rate(0),
        channels(0)
    {

    }
    ~AudioFormatPrivate()
    {

    }

    void setChannels(int cls)
    {
        channels = cls;
        if (av_get_channel_layout_nb_channels(channel_layout_ff) != channels) {
            channel_layout_ff = av_get_default_channel_layout(channels);
            channel_layout = AudioFormat::channelLayoutFromFFmpeg(channel_layout_ff);
        }
    }

    void setChannelLayoutFFmpeg(int64_t clff)
    {
        channel_layout_ff = clff;
        channels = av_get_channel_layout_nb_channels(channel_layout_ff);
    }

    AudioFormat::SampleFormat sample_fmt;
    AVSampleFormat av_sample_fmt;
    AudioFormat::ChannelLayout channel_layout;
    int sample_rate;
    int channels;
    int64_t channel_layout_ff;
};

AudioFormat::AudioFormat():
    d_ptr(new AudioFormatPrivate)
{

}

AudioFormat::~AudioFormat() {

}

AudioFormat::AudioFormat(const AudioFormat &other)
{
    DPTR_D(AudioFormat);
    d_ptr = other.d_ptr;
}

AudioFormat &AudioFormat::operator=(const AudioFormat &other)
{
    DPTR_D(AudioFormat);
    d_ptr = other.d_ptr;
    return *this;
}

bool AudioFormat::operator==(const AudioFormat &other) const
{
    DPTR_D(const AudioFormat);
    return d->sample_fmt == other.d_ptr->sample_fmt &&
            d->channels == other.d_ptr->channels &&
            d->channel_layout == other.d_ptr->channel_layout &&
            d->sample_rate == other.d_ptr->sample_rate;
}

AudioFormat::SampleFormat AudioFormat::sampleFormatFromFFmpeg(int fffmt)
{
    for (int i = 0; samplefmts[i].fmt != AudioFormat::SampleFormat_Unknown; ++i) {
        if ((int)samplefmts[i].avfmt == fffmt)
            return samplefmts[i].fmt;
    }
    return AudioFormat::SampleFormat_Unknown;
}

int AudioFormat::sampleFormatToFFmpeg(AudioFormat::SampleFormat fmt)
{
    for (int i = 0; samplefmts[i].fmt != AudioFormat::SampleFormat_Unknown; ++i) {
        if (samplefmts[i].fmt == fmt)
            return (int)samplefmts[i].avfmt;
    }
    return (int)AV_SAMPLE_FMT_NONE;
}

AudioFormat::ChannelLayout AudioFormat::channelLayoutFromFFmpeg(int64_t clff) {
    for (int i = 0; i < sizeof(channelLayoutMap) / sizeof(channelLayoutMap[0]); ++i) {
        if (channelLayoutMap[i].ff == clff)
            return channelLayoutMap[i].cl;
    }
    return ChannelLayout_Unsupported;
}

int64_t AudioFormat::channelLayoutToFFmpeg(AudioFormat::ChannelLayout cl) {
    for (int i = 0; i < sizeof(channelLayoutMap) / sizeof(channelLayoutMap[0]); ++i) {
        if (channelLayoutMap[i].cl == cl)
            return channelLayoutMap[i].ff;
    }
    return 0;
}

bool AudioFormat::isValid() const {
    DPTR_D(const AudioFormat);
    return d->sample_fmt > 0 &&
            (d->channels > 0 || d->channel_layout > 0) &&
            d->sample_fmt != AudioFormat::SampleFormat_Unknown;
}

bool AudioFormat::isFloat() const {
    DPTR_D(const AudioFormat);
    return !!(d->sample_fmt & kFloat);
}

bool AudioFormat::isPlanar() const {
    DPTR_D(const AudioFormat);
    return !!(d->sample_fmt & kPlanar);
}

bool AudioFormat::isUnsigned() const {
    DPTR_D(const AudioFormat);
    return !!(d->sample_fmt & kUnsigned);
}

int AudioFormat::channels() const {
    DPTR_D(const AudioFormat);
    return d->channels;
}

void AudioFormat::setChannels(int cls) {
    DPTR_D(AudioFormat);
    d->setChannels(cls);
}

int AudioFormat::bytesPerSample() const {
    DPTR_D(const AudioFormat);
    return d->sample_fmt & (((1 << (kSize + 1)) - 1));
}

int AudioFormat::bytesPerFrame() const {
    if (!isValid())
        return 0;
    return bytesPerSample() * channels();
}

int AudioFormat::bytesPerSecond() const
{
    DPTR_D(const AudioFormat);
    return d->channels * d->sample_rate * bytesPerSample();
}

int64_t AudioFormat::durationForBytes(int64_t bytes) {
    DPTR_D(const AudioFormat);
    if (!isValid() || bytes <= 0)
        return 0;
    return int64_t(kHz * (bytes / bytesPerFrame())) / sampleRate();
}

void AudioFormat::setSampleFormat(AudioFormat::SampleFormat sampleFormat)
{
    DPTR_D(AudioFormat);
    d->sample_fmt = sampleFormat;
    d->av_sample_fmt = (AVSampleFormat)AudioFormat::sampleFormatToFFmpeg(sampleFormat);
}

AudioFormat::SampleFormat AudioFormat::sampleFormat() const
{
    DPTR_D(const AudioFormat);
    return d->sample_fmt;
}

void AudioFormat::setSampleFormatFFmpeg(int ffSampleFormat)
{
    DPTR_D(AudioFormat);
    d->av_sample_fmt = (AVSampleFormat)ffSampleFormat;
    d->sample_fmt = AudioFormat::sampleFormatFromFFmpeg(ffSampleFormat);
}

int AudioFormat::sampleFormatFFmpeg() const
{
    DPTR_D(const AudioFormat);
    return d->av_sample_fmt;
}

void AudioFormat::setChannelLayoutFFmpeg(int64_t layout)
{
    DPTR_D(AudioFormat);
    d->channel_layout = AudioFormat::channelLayoutFromFFmpeg(layout);
    d->setChannelLayoutFFmpeg(layout);
}

int64_t AudioFormat::channelLayoutFFmpeg() const
{
    DPTR_D(const AudioFormat);
    return d->channel_layout_ff;
}

void AudioFormat::setChannelLayout(AudioFormat::ChannelLayout layout)
{
    DPTR_D(AudioFormat);
    d->channel_layout = layout;
    d->setChannelLayoutFFmpeg(AudioFormat::channelLayoutToFFmpeg(layout));
}

AudioFormat::ChannelLayout AudioFormat::channelLayout() const
{
    DPTR_D(const AudioFormat);
    return d->channel_layout;
}

int AudioFormat::sampleRate() const {
    DPTR_D(const AudioFormat);
    return d->sample_rate;
}

void AudioFormat::setSampleRate(int rate) {
    DPTR_D(AudioFormat);
    d->sample_rate = rate;
}

int AudioFormat::planeCount() const {
    return isPlanar() ? channels() : 1;
}

AudioFormat::SampleFormat AudioFormat::make(int bytesPerSample, bool isFloat, bool isUnsigned, bool isPlanar)
{
    int f = bytesPerSample;
    if (isFloat)
        f |= kFloat;
    if (isUnsigned)
        f |= kUnsigned;
    if (isPlanar)
        f |= kPlanar;
    return SampleFormat(f);
}

NAMESPACE_END
