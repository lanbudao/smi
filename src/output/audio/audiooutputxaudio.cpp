#include <windows.h>
#include <sstream>
#include <algorithm>

#include "AudioOutputBackend.h"
#include "AudioOutputBackend_p.h"
#include "AVLog.h"
#include "AudioFormat.h"
#include "Factory.h"
#include "mkid.h"
#include "util/semaphore.h"
#include "xaudio2_compat.h"

#define _WIN32_DCOM
#ifndef XA_LOG_COMPONENT
#define XA_LOG_COMPONENT "XAudio2"
#endif //DX_LOG_COMPONENT
#define XA_ENSURE(f, ...) XA_CHECK(f, return __VA_ARGS__;)
#define XA_WARN(f) XA_CHECK(f)
#define XA_CHECK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            AVWarning(XA_LOG_COMPONENT " error@%d. " #f ": (%ld)\n",\
              __LINE__, hr); \
            __VA_ARGS__ \
        } \
    } while (0)

#define SAFE_RELEASE(p) if (p) {p->Release(); p = nullptr;}

NAMESPACE_BEGIN

extern int channelLayoutToMS(int64_t av);

class AudioOutputXAudioPrivate : public AudioOutputBackendPrivate
{
public:

};

class AudioOutputXAudio PU_NO_COPY : public AudioOutputBackend, public IXAudio2VoiceCallback
{
    DPTR_DECLARE_PRIVATE(AudioOutputXAudio)
public:
    AudioOutputXAudio();
    ~AudioOutputXAudio() PU_DECL_OVERRIDE;

    bool initialize();
    bool uninitialize();
    // TODO: check channel layout. xaudio2 supports channels>2
    bool isSupported(const AudioFormat& format) const PU_DECL_OVERRIDE;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const PU_DECL_OVERRIDE;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const PU_DECL_OVERRIDE;
    BufferControl bufferControl() const;

    bool open() PU_DECL_OVERRIDE;
    bool close() PU_DECL_OVERRIDE;
    bool write(const char* data, int size) PU_DECL_OVERRIDE;

    void onCallback();
    bool play() PU_DECL_OVERRIDE;

    bool setVolume(float value) PU_DECL_OVERRIDE;
    float volume() const PU_DECL_OVERRIDE;

public:
    STDMETHOD_(void, OnVoiceProcessingPassStart)(THIS_ UINT32 bytesRequired) PU_DECL_OVERRIDE { PU_UNUSED(bytesRequired); }
    STDMETHOD_(void, OnVoiceProcessingPassEnd)(THIS) PU_DECL_OVERRIDE {}
    STDMETHOD_(void, OnStreamEnd)(THIS) PU_DECL_OVERRIDE {}
    STDMETHOD_(void, OnBufferStart)(THIS_ void* bufferContext) PU_DECL_OVERRIDE { PU_UNUSED(bufferContext); }
    STDMETHOD_(void, OnBufferEnd)(THIS_ void* bufferContext) PU_DECL_OVERRIDE {
        AudioOutputXAudio* ao = reinterpret_cast<AudioOutputXAudio*>(bufferContext);
        if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
            ao->onCallback();
        }
    }
    STDMETHOD_(void, OnLoopEnd)(THIS_ void* bufferContext) PU_DECL_OVERRIDE { PU_UNUSED(bufferContext); }
    STDMETHOD_(void, OnVoiceError)(THIS_ void* bufferContext, HRESULT error) PU_DECL_OVERRIDE {
        PU_UNUSED(bufferContext);
        AVWarning() << __FUNCTION__ << ":onVoiceError (" << error << ") \n";
    }
private:
    bool xaudio2_winsdk;
    bool uninit_com;
    // TODO: com ptr
    IXAudio2SourceVoice* source_voice;
    union {
        struct {
            DXSDK::IXAudio2* xaudio;
            DXSDK::IXAudio2MasteringVoice* master;
        } dxsdk;
        struct {
            WinSDK::IXAudio2* xaudio;
            WinSDK::IXAudio2MasteringVoice* master;
        } winsdk;
    };
    Semaphore sem;
    size_t queue_data_write;
    char* queue_data; int queue_data_size;

    HINSTANCE dll;
    std::stringstream dll_name;
};

typedef AudioOutputXAudio AudioOutputBackendXAudio;
static const AudioOutputBackendId AudioOutputBackendId_XAudio = mkid::id32base36_6<'X', 'A', 'u', 'd', 'i', 'o'>::value;
FACTORY_REGISTER(AudioOutputBackend, XAudio, "XAudio")

AudioOutputXAudio::AudioOutputXAudio() :
    AudioOutputBackend(new AudioOutputBackendPrivate),
    uninit_com(false),
    source_voice(nullptr),
    queue_data_write(0),
    queue_data_size(0),
    queue_data(nullptr)
{
    DPTR_D(AudioOutputXAudio);
    memset(&dxsdk, 0, sizeof(dxsdk));
    d->avaliable = false;
    //setDeviceFeatures(AudioOutput::DeviceFeatures()|AudioOutput::SetVolume);

    // https://github.com/wang-bin/QtAV/issues/518
    // already initialized in main thread. If RPC_E_CHANGED_MODE no ref is added, CoUninitialize can lead to crash
    HRESULT res = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    uninit_com = (res != RPC_E_CHANGED_MODE);
    // load dll. <win8: XAudio2_7.DLL, <win10: XAudio2_8.DLL, win10: XAudio2_9.DLL. also defined by XAUDIO2_DLL_A in xaudio2.h
    for (int ver = 9; ver >= 7; ver--) {
        dll_name << "XAudio2_" << ver;
        AVDebug() << "XAudio's version is " << dll_name.str() << "\n";
        dll = LoadLibrary(TEXT(dll_name.str().c_str()));
        if (!dll) {
            AVWarning("Cannot load dll %s\n", dll_name.str());
            continue;
        }
#if (_WIN32_WINNT < _WIN32_WINNT_WIN8)
        // defined as an inline function
        AVDebug("Build with XAudio2 from DXSDK\n");
#else
        AVDebug("Build with XAudio2 from Win8 or later SDK\n");
#endif
        bool ready = false;
        if (!ready && ver >= 8) {
            xaudio2_winsdk = true;
            AVDebug("Try symbol 'XAudio2Create' from WinSDK dll\n");
            typedef HRESULT(__stdcall* XAudio2Create_t)(WinSDK::IXAudio2** ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor);
            XAudio2Create_t XAudio2Create = (XAudio2Create_t)GetProcAddress(dll, "XAudio2Create");
            if (XAudio2Create)
                ready = SUCCEEDED(XAudio2Create(&winsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));
        }
        if (!ready && ver < 8) {
            xaudio2_winsdk = false;
            // try xaudio2 from dxsdk without symbol
            AVDebug("Try inline function 'XAudio2Create' from DXSDK\n");
            ready = SUCCEEDED(DXSDK::XAudio2Create(&dxsdk.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR));

        }
        if (ready)
            break;
        if (dll)
            FreeLibrary(dll);
        dll = nullptr;
    }
    AVDebug("xaudio2: %p\n", winsdk.xaudio);
    d->avaliable = !!(winsdk.xaudio);
}

AudioOutputXAudio::~AudioOutputXAudio()
{
    if (xaudio2_winsdk) {
        SAFE_RELEASE(winsdk.xaudio);
    }
    else {
        SAFE_RELEASE(dxsdk.xaudio);
    }
    if (queue_data) {
        free(queue_data);
        queue_data = nullptr;
    }

    if (uninit_com)
        CoUninitialize();
}

bool AudioOutputXAudio::initialize()
{
    DPTR_D(AudioOutputXAudio);
    
    return true;
}

bool AudioOutputXAudio::uninitialize()
{

    return true;
}

bool AudioOutputXAudio::isSupported(const AudioFormat& format) const
{
    return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());
}

bool AudioOutputXAudio::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return !IsPlanar(sampleFormat) && RawSampleSize(sampleFormat) < sizeof(double); // TODO: what about s64?
}

bool AudioOutputXAudio::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stereo;
}

AudioOutputBackend::BufferControl AudioOutputXAudio::bufferControl() const
{
    return CountCallback;
}

bool AudioOutputXAudio::open()
{
    DPTR_D(AudioOutputXAudio);

    if (!d->avaliable)
        return false;
    WAVEFORMATEX wf;
    wf.cbSize = 0;
    wf.nChannels = d->format.channels();
    wf.nSamplesPerSec = d->format.sampleRate(); // FIXME: use supported values
    wf.wFormatTag = d->format.isFloat() ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
    wf.wBitsPerSample = d->format.bytesPerSample() * 8;
    wf.nBlockAlign = wf.nChannels * d->format.bytesPerSample();
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    // TODO: channels >2, see dsound
    const UINT32 flags = 0; //XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH;
    // TODO: sdl freq 1.0
    if (xaudio2_winsdk) {
        // TODO: device Id property
        // TODO: parameters now default.
        XA_ENSURE(winsdk.xaudio->CreateMasteringVoice(&winsdk.master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE), false);
        XA_ENSURE(winsdk.xaudio->CreateSourceVoice(&source_voice, &wf, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, NULL, NULL), false);
        XA_ENSURE(winsdk.xaudio->StartEngine(), false);
    }
    else {
        XA_ENSURE(dxsdk.xaudio->CreateMasteringVoice(&dxsdk.master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE), false);
        XA_ENSURE(dxsdk.xaudio->CreateSourceVoice(&source_voice, &wf, flags, XAUDIO2_DEFAULT_FREQ_RATIO, this, NULL, NULL), false);
        XA_ENSURE(dxsdk.xaudio->StartEngine(), false);
    }
    XA_ENSURE(source_voice->Start(0, XAUDIO2_COMMIT_NOW), false);
    AVDebug("source_voice:%p\n", source_voice);

    queue_data_size = d->buffer_size * d->buffer_count;
    queue_data = (char*)malloc(queue_data_size);
    sem.release(d->buffer_count - sem.available());
    return true;
}

bool AudioOutputXAudio::close()
{
    AVDebug("source_voice: %p, master: %p\n", source_voice, winsdk.master);
    if (source_voice) {
        source_voice->Stop(0, XAUDIO2_COMMIT_NOW);
        source_voice->FlushSourceBuffers();
        source_voice->DestroyVoice();
        source_voice = NULL;
    }
    if (xaudio2_winsdk) {
        if (winsdk.master) {
            winsdk.master->DestroyVoice();
            winsdk.master = NULL;
        }
        if (winsdk.xaudio)
            winsdk.xaudio->StopEngine();
    }
    else {
        if (dxsdk.master) {
            dxsdk.master->DestroyVoice();
            dxsdk.master = NULL;
        }
        if (dxsdk.xaudio)
            dxsdk.xaudio->StopEngine();
    }

    //queue_data.clear();
    if (queue_data) {
        free(queue_data);
        queue_data = nullptr;
    }
    queue_data_write = 0;
    return true;
}
bool AudioOutputXAudio::write(const char* data, int size)
{
    //qDebug("sem: %d, write: %d/%d", sem.available(), queue_data_write, queue_size);
    if (bufferControl() & CountCallback)
        sem.acquire();
    const int s = min(queue_data_size - queue_data_write, size);
    // assume size <= buffer_size. It's true in QtAV
    if (s < size)
        queue_data_write = 0;
    memcpy(queue_data + queue_data_write, data, size);
    XAUDIO2_BUFFER xb; //IMPORTANT! wrong value(playbegin/length, loopbegin/length) will result in commit sourcebuffer fail
    memset(&xb, 0, sizeof(XAUDIO2_BUFFER));
    xb.AudioBytes = size;
    //xb.Flags = XAUDIO2_END_OF_STREAM;
    xb.pContext = this;
    xb.pAudioData = (const BYTE*)(queue_data + queue_data_write);
    queue_data_write += size;
    if (queue_data_write == queue_data_size)
        queue_data_write = 0;
    XA_ENSURE(source_voice->SubmitSourceBuffer(&xb, NULL), false);
    // TODO: XAUDIO2_E_DEVICE_INVALIDATED
    return true;
}

void AudioOutputXAudio::onCallback()
{
    if (bufferControl() & CountCallback)
        sem.release();
}

bool AudioOutputXAudio::play()
{
    return true;
}

bool AudioOutputXAudio::setVolume(float value)
{
    // master or source?
    XA_ENSURE(source_voice->SetVolume(value), false);
    return true;
}

float AudioOutputXAudio::volume() const
{
    FLOAT value;
    source_voice->GetVolume(&value);
    return value;
}

NAMESPACE_END