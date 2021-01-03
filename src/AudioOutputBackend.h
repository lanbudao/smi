#ifndef AUDIO_OUTPUT_BACKEND_H
#define AUDIO_OUTPUT_BACKEND_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "AudioFormat.h"
#include <vector>
#include <string>

typedef int AudioOutputBackendId;

NAMESPACE_BEGIN

class AudioOutputBackendPrivate;
class AudioOutputBackend
{
    DPTR_DECLARE_PRIVATE(AudioOutputBackend)
public:
    virtual ~AudioOutputBackend();

    static std::vector<std::string> defaultPriority();
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool clear() {return false;}
    virtual bool write(const char *data, int size) = 0;
    virtual bool play() { return true; }
    virtual bool pause(bool flag = true) { return true; }
    /**
     * @brief The BufferControl enum
     * Used to adapt to different audio playback backend. Usually you don't need this in application level development.
    */
    enum BufferControl {
        User = 0,    // You have to reimplement waitForNextBuffer()
        Blocking = 1,
        BytesCallback = 1 << 1,
        CountCallback = 1 << 2,
        PlayedCount = 1 << 3, //number of buffers played since last buffer dequeued
        PlayedBytes = 1 << 4,
        OffsetIndex = 1 << 5, //current playing offset
        OffsetBytes = 1 << 6, //current playing offset by bytes
        WritableBytes = 1 << 7,
    };
    virtual BufferControl bufferControl() const = 0;

    bool avaliable() const;
    void setAvaliable(bool b);
    void setFormat(const AudioFormat &fmt);
    void setBufferSize(int size);
    void setBufferCount(int count);
    virtual void acquireNextBuffer() {}

    virtual int getOffsetByBytes() { return -1; }// OffsetBytes

    virtual bool isSupported(const AudioFormat& format) const { return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());}
    // FIXME: workaround. planar convertion crash now!
    virtual bool isSupported(AudioFormat::SampleFormat f) const { return !IsPlanar(f);}
    // 5, 6, 7 channels may not play
    virtual bool isSupported(AudioFormat::ChannelLayout cl) const { return int(cl) < int(AudioFormat::ChannelLayout_Unsupported);}

    virtual float volume() const { return 1.0; }
    virtual bool setVolume(float value) { return false; };

public:
    template<class C> static bool Register(AudioOutputBackendId id, const char* name) { return Register(id, create<C>, name);}
    static AudioOutputBackend* create(AudioOutputBackendId id);
    static AudioOutputBackend* create(const char* name);
    static AudioOutputBackendId* next(AudioOutputBackendId* id = 0);
    static const char* name(AudioOutputBackendId id);
    static AudioOutputBackendId id(const char* name);
    static size_t count();

private:
    template<class C> static AudioOutputBackend* create() { return new C();}
    typedef AudioOutputBackend* (*AudioOutputBackendCreator)();
    static bool Register(AudioOutputBackendId id, AudioOutputBackendCreator, const char *name);

protected:
    const AudioFormat *format();

protected:
    AudioOutputBackend(AudioOutputBackendPrivate* p);
    DPTR_DECLARE(AudioOutputBackend);
};

NAMESPACE_END
#endif //AUDIO_OUTPUT_BACKEND_H
