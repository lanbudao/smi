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
    AudioOutputBackend();
    virtual ~AudioOutputBackend();

    static std::vector<std::string> defaultPriority();
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool clear() {return false;}
    virtual bool write(const char *data, int size) = 0;
    virtual bool play() { return true; }

    bool avaliable() const;
    void setAvaliable(bool b);
    void setFormat(const AudioFormat &fmt);

    virtual bool isSupported(const AudioFormat& format) const { return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());}
    // FIXME: workaround. planar convertion crash now!
    virtual bool isSupported(AudioFormat::SampleFormat f) const { return !IsPlanar(f);}
    // 5, 6, 7 channels may not play
    virtual bool isSupported(AudioFormat::ChannelLayout cl) const { return int(cl) < int(AudioFormat::ChannelLayout_Unsupported);}

public:
    template<class C> static bool Register(AudioOutputBackendId id, const char* name) { return Register(id, create<C>, name);}
    static AudioOutputBackend* create(AudioOutputBackendId id);
    static AudioOutputBackend* create(const char* name);
    static AudioOutputBackendId* next(AudioOutputBackendId* id = 0);
    static const char* name(AudioOutputBackendId id);
    static AudioOutputBackendId id(const char* name);

private:
    template<class C> static AudioOutputBackend* create() { return new C();}
    typedef AudioOutputBackend* (*AudioOutputBackendCreator)();
    static bool Register(AudioOutputBackendId id, AudioOutputBackendCreator, const char *name);

protected:
    const AudioFormat *format();

protected:
    DPTR_DECLARE(AudioOutputBackend)
};

NAMESPACE_END
#endif //AUDIO_OUTPUT_BACKEND_H
