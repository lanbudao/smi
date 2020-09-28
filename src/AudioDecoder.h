#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "AVDecoder.h"
#include "AudioFrame.h"
#include "Packet.h"

NAMESPACE_BEGIN

typedef int AudioDecoderId;
class AudioDecoderPrivate;
class FFPRO_EXPORT AudioDecoder: public AVDecoder
{
    DPTR_DECLARE_PRIVATE(AudioDecoder)
public:
    ~AudioDecoder() PU_DECL_OVERRIDE;

    static StringList supportedCodecs();

    virtual std::string name() const PU_DECL_OVERRIDE;
    virtual std::string description() const PU_DECL_OVERRIDE;

    void setAudioResample(AudioResample *resample);
    AudioResample *audioResample() const;

    static AudioDecoder * create(const char *name = "FFmpeg");
    static AudioDecoder* create(AudioDecoderId id);

    virtual AudioDecoderId id() const = 0;

    virtual AudioFrame frame() = 0;

    template<class T>
    static bool Register(AudioDecoderId id, const char *name) { return Register(id, create<T>, name); }

    /**
     * @brief next
     * @param id NULL to get the first id address
     * @return address of id or NULL if not found/end
     */
    static AudioDecoderId* next(AudioDecoderId* id = 0);
    static const char* name(AudioDecoderId id);
    static AudioDecoderId id(const char* name);

    virtual int decode(const Packet& packet) = 0;

private:
    template<class T>
    static AudioDecoder * create() { return new T(); }
    typedef AudioDecoder* (*AudioDecoderCreator)();

protected:
    static bool Register(AudioDecoderId id, AudioDecoderCreator, const char *name);
    AudioDecoder(AudioDecoderPrivate *d);
};

extern AudioDecoderId AudioDecoderId_FFmpeg;

NAMESPACE_END
#endif //AUDIODECODER_H
