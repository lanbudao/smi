#include "AudioFrame.h"
#include "Frame_p.h"
#include "AudioResample.h"
#include "AVLog.h"

NAMESPACE_BEGIN

class AudioFramePrivate : public FramePrivate {
public:
    AudioFramePrivate():
        samples_per_channel(0),
        resample(nullptr)
    {

    }

    void setFormat(const AudioFormat &fmt)
    {
        format = fmt;
        planes.reserve(fmt.planeCount());
        planes.resize(fmt.planeCount());
        line_sizes.reserve(fmt.planeCount());
        line_sizes.resize(fmt.planeCount());
    }

    AudioFormat format;
    int samples_per_channel;
    AudioResample *resample;
};

AudioFrame::AudioFrame(const AudioFormat &format, const ByteArray &data):
    Frame(new AudioFramePrivate)
{
    DPTR_D(AudioFrame);
    if (!format.isValid())
        return;
    d->setFormat(format);
    if (data.isEmpty())
        return;
    d->data = data;
    d->samples_per_channel = data.size() / d->format.channels() / d->format.bytesPerSample();
    const int nb_planes = d->format.planeCount();
    const int bpl = d->data.size() / nb_planes;
    for (int i = 0; i < nb_planes; ++i) {
        setBytesPerLine(bpl, i);
        setBits((uchar*)d->data.constData() + i * bpl, i);
    }
}

AudioFrame::~AudioFrame()
{

}

bool AudioFrame::isValid() const {
    DPTR_D(const AudioFrame);
    return d->format.isValid() && d->samples_per_channel > 0;
}

void AudioFrame::setData(void *data)
{
    DPTR_D(AudioFrame);
    AVFrame* f = reinterpret_cast<AVFrame*>(data);
    av_frame_move_ref(d->frame, f);
    AudioFormat fmt;
    fmt.setSampleFormatFFmpeg(d->frame->format);
    fmt.setChannelLayoutFFmpeg(d->frame->channel_layout);
    fmt.setSampleRate(d->frame->sample_rate);
    fmt.setChannels(d->frame->channels);
    d->setFormat(fmt);
    setBits(d->frame->extended_data);
    setBytesPerLine(d->frame->linesize[0], 0);
    setSamplePerChannel(d->frame->nb_samples);
    setTimestamp(static_cast<double>(d->frame->pts) / 1000.0);
}

int AudioFrame::channelCount() const {
    DPTR_D(const AudioFrame);
    if (d->format.isValid())
        return d->format.channels();
    return 0;
}

int64_t AudioFrame::duration() {
    DPTR_D(AudioFrame);
    return d->format.durationForBytes(d->data.size());
}

void AudioFrame::setFormat(const AudioFormat & f)
{
    d_func()->setFormat(f);
}

AudioFormat AudioFrame::format() const {
    DPTR_D(const AudioFrame);
    return d->format;
}

int AudioFrame::samplePerChannel() const {
    DPTR_D(const AudioFrame);
    return d->samples_per_channel;
}

void AudioFrame::setSamplePerChannel(int channel) {
    DPTR_D(AudioFrame);
    if (!d->format.isValid())
        return;
    d->samples_per_channel = channel;
    int nb_planes = d->format.planeCount();
    int bytesPerLine = 0;

    if (d->line_sizes.size() <= 0)
        return;
    if (d->line_sizes[0] > 0) {
        bytesPerLine = d->line_sizes[0];
    } else {
        bytesPerLine = d->samples_per_channel *
                       d->format.bytesPerSample() *
                       d->format.isPlanar() ? 1 : d->format.channels();
    }
    for (int i = 0; i < nb_planes; ++i) {
        setBytesPerLine(bytesPerLine, i);
    }
    if (d->data.isEmpty())
        return;
    if (!constBits(0))
        setBits((uchar *)d->data.constData(), 0);
    for (int i = 1; i < nb_planes; ++i) {
        if (!constBits(i)) {
            setBits((uchar *)constBits(i - 1) + bytesPerLine, i);
        }
    }
}

void AudioFrame::setAudioResampler(AudioResample *resample)
{
    DPTR_D(AudioFrame);
    d->resample = resample;
}

AudioFrame AudioFrame::to(const AudioFormat &fmt) const
{
    if (!isValid() || !constBits(0))
        return AudioFrame();
    DPTR_D(const AudioFrame);
    AudioResample *resample = d->resample;
    if (!resample) {
        AVWarning("No audio resampler found!\n");
        return AudioFrame();
    }
    resample->setInFormat(d->format);
    resample->setOutFormat(fmt);
    resample->setInSamplesPerChannel(samplePerChannel());
    if (!(resample->convert((const uchar **)d->planes.data()))) {
//        AVError("Audio frame convert failed!\n");
        return AudioFrame();
    }
    AudioFrame frame(fmt, resample->outData());
    frame.setSamplePerChannel(resample->outSamplesPerChannel());
    frame.setTimestamp(d->timestamp);
    frame.setSerial(d->serial);
    return frame;
}

int AudioFrame::dataSize() const
{
    DPTR_D(const AudioFrame);
    return av_samples_get_buffer_size(NULL, d->format.channels(),
        samplePerChannel(),
        (AVSampleFormat)d->format.sampleFormatFFmpeg(), 1);
}

NAMESPACE_END
