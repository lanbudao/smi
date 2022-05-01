#include "AudioOutputBackend.h"
#include "AudioOutputBackend_p.h"
#include "portaudio.h"
#include "AVLog.h"
#include "AudioFormat.h"
#include "Factory.h"
#include "mkid.h"

#define PA_ENSURE_OK(f, ...) PA_CHECK(f, return __VA_ARGS__;)
#define PA_CHECK(f, ...) \
    do { \
        PaError err = f; \
        if (err != paNoError) { \
            AVWarning("PortAudio error: %s, on line %d\n", Pa_GetErrorText(err), __LINE__);\
            __VA_ARGS__ \
        } \
    } while (0)

NAMESPACE_BEGIN

class AudioOutputPortAudio PU_NO_COPY: public AudioOutputBackend
{
public:
    AudioOutputPortAudio();
    ~AudioOutputPortAudio() PU_DECL_OVERRIDE;

    bool initialize();
    bool uninitialize();
    BufferControl bufferControl() const;
    PaSampleFormat toPaSampleFormat(const AudioFormat::SampleFormat &format);

    bool open() PU_DECL_OVERRIDE;
    bool close() PU_DECL_OVERRIDE;
    bool write(const char *data, int size) PU_DECL_OVERRIDE;

private:
    PaStreamParameters *stream_paras;
    PaStream *stream;
    bool isInitialized;
    double outputLatency;
};

typedef AudioOutputPortAudio AudioOutputBackendPortAudio;
static const AudioOutputBackendId AudioOutputBackendId_PortAudio = mkid::id32base36_6<'P', 'o', 'r', 't', 'A', 'u'>::value;
FACTORY_REGISTER(AudioOutputBackend, PortAudio, "PortAudio")

AudioOutputPortAudio::AudioOutputPortAudio():
    AudioOutputBackend(new AudioOutputBackendPrivate),
    stream_paras(nullptr),
    isInitialized(false),
    outputLatency(0.0),
    stream(nullptr)
{
    AVDebug("PortAudio' version %d, %s\n", Pa_GetVersion(), Pa_GetVersionText());

    if (!initialize()) {
        return;
    }

    int devNum = Pa_GetDeviceCount();
    for (int i = 0; i < devNum; ++i) {
        const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(i);
        if (!devInfo)
            continue;
        const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(devInfo->hostApi);
        if (!hostApiInfo)
            continue;
        AVDebug("audio device %d=> %s:%s\n", i, devInfo->name, hostApiInfo->name);
        AVDebug("max in/out channel: %d / %d\n", devInfo->maxInputChannels, devInfo->maxOutputChannels);
    }

    /*init stream paras*/
    stream_paras = new PaStreamParameters;
    memset(stream_paras, 0, sizeof(PaStreamParameters));
    stream_paras->device = Pa_GetDefaultOutputDevice();
    if (stream_paras->device == paNoDevice) {
        AVWarning("PortAudio get default device error!\n");
        return;
    }
    const PaDeviceInfo *info = Pa_GetDeviceInfo(stream_paras->device);
    if (!info)
        return;
    AVDebug("default audio device: %s\n", info->name);
    AVDebug("default max in/out channel: %d / %d\n", info->maxInputChannels, info->maxOutputChannels);
    stream_paras->hostApiSpecificStreamInfo = nullptr;
    stream_paras->suggestedLatency = info->defaultHighOutputLatency;
}

AudioOutputPortAudio::~AudioOutputPortAudio()
{
    if (stream_paras) {
        delete stream_paras;
        stream_paras = nullptr;
    }
}

bool AudioOutputPortAudio::initialize()
{
    if (isInitialized)
        return true;
    PA_ENSURE_OK(Pa_Initialize(), false);
    isInitialized = true;
    return true;
}

bool AudioOutputPortAudio::uninitialize()
{
    if (!isInitialized)
        return true;
    PaError error = Pa_Terminate();
	if (error != paNoError) {
		AVWarning("PortAudio error: %s, on line %d\n", Pa_GetErrorText(error), __LINE__);
		return false;
	}
    isInitialized = false;
    return true;
}

PaSampleFormat AudioOutputPortAudio::toPaSampleFormat(const AudioFormat::SampleFormat &format)
{
    switch (format) {
    case AudioFormat::SampleFormat_Unsigned8:
        return paUInt8;
    case AudioFormat::SampleFormat_Signed8:
        return paInt8;
    case AudioFormat::SampleFormat_Signed16:
        return paInt16;
    case AudioFormat::SampleFormat_Signed24:
        return paInt24;
    case AudioFormat::SampleFormat_Signed32:
        return paInt32;
    case AudioFormat::SampleFormat_Float:
        return paFloat32;
    default:
        return paCustomFormat;
    }
}

bool AudioOutputPortAudio::open()
{
    if (!isInitialized) {
        if (!initialize()) {
            return false;
        }
    }
    stream_paras->sampleFormat = toPaSampleFormat(format()->sampleFormat());
    stream_paras->channelCount = format()->channels();
    /*Don't use callback function*/
    PA_ENSURE_OK(Pa_OpenStream(&stream, nullptr, stream_paras, format()->sampleRate(), 0, paNoFlag, nullptr, nullptr), false);
    outputLatency = Pa_GetStreamInfo(stream)->outputLatency;
    return true;
}

bool AudioOutputPortAudio::close()
{
    if (!stream) {
        return uninitialize();
    }
	PaError err;

	if (!Pa_IsStreamStopped(stream)) {
		err = Pa_StopStream(stream);
		if (err != paNoError) {
			AVWarning("Stop portaudio stream error: %s.\n", Pa_GetErrorText(err));
		}
	}
	err = Pa_CloseStream(stream);
	if (err != paNoError) {
		AVWarning("Close portaudio stream error: %s.\n", Pa_GetErrorText(err));
	}
	stream = nullptr;
	return uninitialize();
}

bool AudioOutputPortAudio::write(const char *data, int size)
{
    if (!stream || !stream_paras)
        return false;
    if (Pa_IsStreamStopped(stream))
        Pa_StartStream(stream);
    unsigned long frames = size / (stream_paras->channelCount * format()->bytesPerSample());
    if (frames <= 0)
        return false;
    PaError err = Pa_WriteStream(stream, data, frames);
    if (err != paNoError) {
        AVWarning("PortAudio write stream error: %s\n", Pa_GetErrorText(err));
        return false;
    }
    return true;
}

AudioOutputBackend::BufferControl AudioOutputPortAudio::bufferControl() const
{
    return Blocking;
}

NAMESPACE_END

