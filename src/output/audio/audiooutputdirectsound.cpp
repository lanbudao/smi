#include "sdk/global.h"
#include "AudioOutputBackend.h"
#include "AudioOutputBackend_p.h"
#include "mkid.h"
#include "Factory.h"
#include "AVLog.h"
#include "CThread.h"
#include "utils/semaphore.h"
#include "innermath.h"

#define DIRECTSOUND_VERSION 0x0600
#include <dsound.h>
#include <atomic>

NAMESPACE_BEGIN

#ifndef DX_LOG_COMPONENT
#define DX_LOG_COMPONENT "DirectX"
#endif //DX_LOG_COMPONENT
#define DX_ENSURE(f, ...) DX_CHECK(f, return __VA_ARGS__;)
#define DX_WARN(f) DX_CHECK(f)
#define DX_CHECK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            AVWarning(DX_LOG_COMPONENT " error@%d. " #f ": (%ld)\n",\
              __LINE__, hr); \
            __VA_ARGS__ \
        } \
    } while (0)

#define SAFE_RELEASE(p) if (p) {p->Release(); p = nullptr;}

// use the definitions from the win32 api headers when they define these
#define WAVE_FORMAT_IEEE_FLOAT      0x0003
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#define WAVE_FORMAT_EXTENSIBLE      0xFFFE
/* GUID SubFormat IDs */
/* We need both b/c const variables are not compile-time constants in C, giving
 * us an error if we use the const GUID in an enum */
#undef DEFINE_GUID
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) EXTERN_C const GUID DECLSPEC_SELECTANY name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF, WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_PCM, WAVE_FORMAT_PCM, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(_KSDATAFORMAT_SUBTYPE_UNKNOWN, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
#ifndef MS_GUID
#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#endif //MS_GUID
#ifndef _WAVEFORMATEXTENSIBLE_
typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to zero. */
    } Samples;
    DWORD           dwChannelMask;      /* which channels are */
    /* present in stream  */
    GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
#endif

/* Microsoft speaker definitions. key/values are equal to FFmpeg's */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

int channelMaskToMS(int64_t av) {
    if (av >= (int64_t)SPEAKER_RESERVED)
        return 0;
    return (int)av;
}

int channelLayoutToMS(int64_t av) {
    return channelMaskToMS(av);
}
class AudioOutputDirectSoundPrivate : public AudioOutputBackendPrivate
{
public:

};
class AudioOutputDirectSound PU_NO_COPY : public AudioOutputBackend
{
    DPTR_DECLARE_PRIVATE(AudioOutputDirectSound)
public:
    AudioOutputDirectSound();
    ~AudioOutputDirectSound() PU_DECL_OVERRIDE;

    bool initialize();
    bool uninitialize();
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    BufferControl bufferControl() const;
    void acquireNextBuffer();
    int getOffsetByBytes();
    bool loadDsound();
    bool unloadDsound();
    bool createBuffer();

    bool open() PU_DECL_OVERRIDE;
    bool close() PU_DECL_OVERRIDE;
    bool write(const char *data, int size) PU_DECL_OVERRIDE;
    bool play() PU_DECL_OVERRIDE;
    bool pause(bool flag = true) override;
    bool avaliable() const { return d_func()->avaliable; }
    HANDLE notifyEvent() { return notify_event; }
    void onCallback();
    float volume() const override;
    bool setVolume(float value) override;

private:
    bool isInitialized;
    double outputLatency;
    HINSTANCE dll;
    LPDIRECTSOUND dsound;              ///direct sound object
    LPDIRECTSOUNDBUFFER prim_buf;      ///primary direct sound buffer
    LPDIRECTSOUNDBUFFER stream_buf;    ///secondary direct sound buffer (stream buffer)
    LPDIRECTSOUNDNOTIFY notify;
    int write_offset;               ///offset of the write cursor in the direct sound buffer
    HANDLE notify_event;
    Semaphore sem;
    std::atomic<int> buffers_free;
    bool paused;

    class PositionWatcher : public CThread {
        AudioOutputDirectSound *ao;
    public:
        PositionWatcher(AudioOutputDirectSound* dsound) : ao(dsound) {}
        void run() {
            DWORD dwResult = 0;
            while (ao->avaliable()) {
                dwResult = WaitForSingleObjectEx(ao->notifyEvent(), 20, FALSE);
                if (dwResult != WAIT_OBJECT_0) {
                    //AVWarning("WaitForSingleObjectEx for ao->notify_event error: %#lx", dwResult);
                    continue;
                }
                ao->onCallback();
            }
        }
    };
    PositionWatcher watcher;
};
typedef AudioOutputDirectSound AudioOutputBackendDirectSound;
static const AudioOutputBackendId AudioOutputBackendId_DirectSound = mkid::id32base36_6<'D', 'i', 'r', 'e', 'c', 't'>::value;
FACTORY_REGISTER(AudioOutputBackend, DirectSound, "DirectSound")

AudioOutputDirectSound::AudioOutputDirectSound() :
    AudioOutputBackend(new AudioOutputDirectSoundPrivate),
    dll(nullptr),
    dsound(nullptr),
    prim_buf(nullptr),
    stream_buf(nullptr),
    notify(nullptr),
    notify_event(nullptr),
    write_offset(0),
    watcher(this),
    paused(false)
{

}

AudioOutputDirectSound::~AudioOutputDirectSound()
{
    watcher.wait();
}

bool AudioOutputDirectSound::initialize()
{
    if (!loadDsound())
        return false;
    typedef HRESULT(WINAPI *DirectSoundCreateFunc)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
    //typedef HRESULT (WINAPI *DirectSoundEnumerateFunc)(LPDSENUMCALLBACKA, LPVOID);
    DirectSoundCreateFunc dsound_create = (DirectSoundCreateFunc)GetProcAddress(dll, "DirectSoundCreate");
    //DirectSoundEnumerateFunc dsound_enumerate = (DirectSoundEnumerateFunc)GetProcAddress(dll, "DirectSoundEnumerateA");
    if (!dsound_create) {
        AVWarning("Failed to resolve 'DirectSoundCreate'\n");
        unloadDsound();
        return false;
    }
    DX_ENSURE(dsound_create(NULL/*dev guid*/, &dsound, NULL), false);
    /*
     * DSSCL_EXCLUSIVE: can modify the settings of the primary buffer,
     * only the sound of this app will be hearable when it will have the focus.
     */
    DX_ENSURE(dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_EXCLUSIVE), false);
    AVDebug("DirectSound initialized.\n");
    DSCAPS dscaps;
    memset(&dscaps, 0, sizeof(DSCAPS));
    dscaps.dwSize = sizeof(DSCAPS);
    DX_ENSURE(dsound->GetCaps(&dscaps), false);
    if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
        AVDebug("DirectSound is emulated\n");
    write_offset = 0;
    return true;
}

bool AudioOutputDirectSound::uninitialize()
{
    SAFE_RELEASE(notify);
    SAFE_RELEASE(prim_buf);
    SAFE_RELEASE(stream_buf);
    SAFE_RELEASE(dsound);
    unloadDsound();
    return true;
}

bool AudioOutputDirectSound::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return !IsPlanar(sampleFormat);
}

AudioOutputBackend::BufferControl AudioOutputDirectSound::bufferControl() const
{
    //return CountCallback;
    return OffsetBytes;
}

void AudioOutputDirectSound::acquireNextBuffer()
{
    //if (bufferControl() & CountCallback) {
    //    sem.acquire();
    //}
    //else {

    //}
}

int AudioOutputDirectSound::getOffsetByBytes()
{
    DWORD read_offset = 0;
    stream_buf->GetCurrentPosition(&read_offset /*play*/, NULL /*write*/); //what's this write_offset?
    return (int)read_offset;
}

bool AudioOutputDirectSound::loadDsound()
{
    dll = LoadLibrary(TEXT("dsound.dll"));
    if (!dll) {
        AVWarning("Can not load dsound.dll!\n");
        return false;
    }
    return true;
}
bool AudioOutputDirectSound::unloadDsound()
{
    if (dll)
        FreeLibrary(dll);
    dll = nullptr;
    return true;
}

/**
 * Creates a DirectSound buffer of the required format.
 *
 * This function creates the buffer we'll use to play audio.
 * In DirectSound there are two kinds of buffers:
 * - the primary buffer: which is the actual buffer that the soundcard plays
 * - the secondary buffer(s): these buffers are the one actually used by
 *    applications and DirectSound takes care of mixing them into the primary.
 *
 * Once you create a secondary buffer, you cannot change its format anymore so
 * you have to release the current one and create another.
 */
bool AudioOutputDirectSound::createBuffer()
{
    DPTR_D(AudioOutputDirectSound);
    WAVEFORMATEXTENSIBLE wformat;
    // TODO:  Dolby Digital AC3
    ZeroMemory(&wformat, sizeof(WAVEFORMATEXTENSIBLE));
    WAVEFORMATEX wf;
    wf.cbSize = 0;
    wf.nChannels = d->format.channels();
    wf.nSamplesPerSec = d->format.sampleRate(); // FIXME: use supported values
    wf.wBitsPerSample = d->format.bytesPerSample() * 8;
    wf.nBlockAlign = wf.nChannels * d->format.bytesPerSample();
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    wformat.dwChannelMask = channelLayoutToMS(d->format.channelLayoutFFmpeg());
    if (d->format.channels() > 2) {
        wf.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        wf.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
        wformat.Samples.wValidBitsPerSample = wf.wBitsPerSample;
    }
    if (d->format.isFloat()) {
        wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        wformat.Samples.wValidBitsPerSample = wf.wBitsPerSample;
    }
    else {
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wformat.SubFormat = _KSDATAFORMAT_SUBTYPE_PCM;
    }
    wformat.Format = wf;

    DSBUFFERDESC dsbdesc;
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 /** Better position accuracy */
        | DSBCAPS_GLOBALFOCUS       /** Allows background playing */
        | DSBCAPS_CTRLVOLUME       /** volume control enabled */
        | DSBCAPS_CTRLPOSITIONNOTIFY;
    dsbdesc.dwBufferBytes = d->buffer_size * d->buffer_count;
    dsbdesc.lpwfxFormat = (WAVEFORMATEX *)&wformat;
    // Needed for 5.1 on emu101k - shit soundblaster
    if (d->format.channels() > 2)
        dsbdesc.dwFlags |= DSBCAPS_LOCHARDWARE;
    // now create the stream buffer (secondary buffer)
    HRESULT res = dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL);
    if (res != DS_OK) {
        if (dsbdesc.dwFlags & DSBCAPS_LOCHARDWARE) {
            // Try without DSBCAPS_LOCHARDWARE
            dsbdesc.dwFlags &= ~DSBCAPS_LOCHARDWARE;
            DX_ENSURE(dsound->CreateSoundBuffer(&dsbdesc, &stream_buf, NULL), (uninitialize() && false));
        }
    }
    AVDebug("Secondary (stream)buffer created\n");
    MS_GUID(IID_IDirectSoundNotify, 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);
    DX_ENSURE(stream_buf->QueryInterface(IID_IDirectSoundNotify, (void**)&notify), false);
    notify_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    std::vector<DSBPOSITIONNOTIFY> notification(d->buffer_count);
    for (int i = 0; i < d->buffer_count; ++i) {
        notification[i].dwOffset = d->buffer_size * i;
        notification[i].hEventNotify = notify_event;
    }
    //notification[buffer_count].dwOffset = DSBPN_OFFSETSTOP;
    //notification[buffer_count].hEventNotify = stop_notify_event;
    DX_ENSURE(notify->SetNotificationPositions(notification.size(), notification.data()), false);
    d->avaliable = true;
    watcher.start();
    sem.release(d->buffer_count - sem.available());
    return true;
}

bool AudioOutputDirectSound::open()
{
    if (initialize() && createBuffer())
        return true;
    unloadDsound();
    SAFE_RELEASE(dsound);
    return false;
}

bool AudioOutputDirectSound::close()
{
    DPTR_D(AudioOutputDirectSound);
    if (!d->avaliable)
        return true;
    d->avaliable = false;
    uninitialize();
    CloseHandle(notify_event);
    notify_event = nullptr;
    return true;
}

bool AudioOutputDirectSound::write(const char *data, int size)
{
    DPTR_D(AudioOutputDirectSound);
    if (paused)
        return false;
    if (bufferControl() & CountCallback) {
        sem.acquire();
    }
    else {
        if (buffers_free.load() <= d->buffer_count)
            buffers_free++;
    }
    LPVOID buffer1 = NULL, buffer2 = NULL;
    DWORD size1 = 0, size2 = 0;
    HRESULT res = stream_buf->Lock(write_offset, size, &buffer1, &size1, &buffer2, &size2, 0); //DSBLOCK_ENTIREBUFFER
    if (res == DSERR_BUFFERLOST) {
        AVDebug("Dsound Buffer Lost\n");
        DX_ENSURE(stream_buf->Restore(), false);
        DX_ENSURE(stream_buf->Lock(write_offset, size, &buffer1, &size1, &buffer2, &size2, 0), false);
    }
    if (!SUCCEEDED(res))
        return false;
    CopyMemory(buffer1, data, size1);
    write_offset += size1;
    if (buffer2)
        CopyMemory(buffer2, data + size1, size2);
    write_offset += size2;
    write_offset %= (d->buffer_size * d->buffer_count);
    DX_ENSURE(stream_buf->Unlock(buffer1, size1, buffer2, size2), false);
    return true;
}

bool AudioOutputDirectSound::play()
{
    DWORD status;
    stream_buf->GetStatus(&status);
    if (!(status & DSBSTATUS_PLAYING)) {
        //must be DSBPLAY_LOOPING. Sound will be very slow if set to 0. I was fucked for a long time. DAMN!
        stream_buf->Play(0, 0, DSBPLAY_LOOPING);
    }
    return true;
}

bool AudioOutputDirectSound::pause(bool flag)
{
    DWORD status;

    paused = flag;
    stream_buf->GetStatus(&status);
    /* pause */
    if (flag) {
        if (status & DSBSTATUS_PLAYING) {
            stream_buf->Stop();
        }
    }
    /* continue */
    else {
        DWORD dwPlayCursor, dwWriteCursor;
        stream_buf->GetCurrentPosition(&dwPlayCursor, &dwWriteCursor);
        stream_buf->SetCurrentPosition(dwPlayCursor);
        if (stream_buf->Play(0, 0, DSBPLAY_LOOPING) == DSERR_BUFFERLOST) {
            AVWarning("Dsound Buffer Lost\n");
        }
    }
    return true;
}

void AudioOutputDirectSound::onCallback()
{
    DPTR_D(AudioOutputDirectSound);
    if (bufferControl() & CountCallback) {
        //AvDebug("callback: %d\n", sem.available());
        if (sem.available() < d->buffer_count) {
            sem.release();
            return;
        }
    }
}

float AudioOutputDirectSound::volume() const
{
    LONG vol = 0;
    DX_ENSURE(stream_buf->GetVolume(&vol), 1.0);
    return FORCE_FLOAT(std::pow(10.0, double(vol - DSBVOLUME_MIN) / 5000.0) / 100.0);
}

bool AudioOutputDirectSound::setVolume(float value)
{
    // dsound supports [0, 1]
    const LONG vol = value <= 0 ? DSBVOLUME_MIN : LONG(std::log10(value*100.0) * 5000.0) + DSBVOLUME_MIN;
    // +DSBVOLUME_MIN == -100dB
    DX_ENSURE(stream_buf->SetVolume(vol), false);
    return true;
}

NAMESPACE_END
