#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "AVDecoder.h"
#include "AudioFrame.h"
#include "Packet.h"
#include "Subtitle.h"

NAMESPACE_BEGIN

typedef int SubtitleDecoderId;
class SubtitleDecoderPrivate;
class SubtitleDecoder: public AVDecoder
{
    DPTR_DECLARE_PRIVATE(SubtitleDecoder)
public:
    ~SubtitleDecoder() PU_DECL_OVERRIDE;

    virtual StringList supportedCodecs() = 0;
    virtual SubtitleFrame processLine(const ByteArray &data, double pts, double duration) = 0;

    virtual std::string name() const PU_DECL_OVERRIDE;
    virtual std::string description() const PU_DECL_OVERRIDE;

    static SubtitleDecoder * create(const char *name = "FFmpeg");
    static SubtitleDecoder* create(SubtitleDecoderId id);

    virtual SubtitleDecoderId id() const = 0;

    template<class T>
    static bool Register(SubtitleDecoderId id, const char *name) { return Register(id, create<T>, name); }

    /**
     * @brief next
     * @param id NULL to get the first id address
     * @return address of id or NULL if not found/end
     */
    static SubtitleDecoderId* next(SubtitleDecoderId* id = 0);
    static const char* name(SubtitleDecoderId id);
    static SubtitleDecoderId id(const char* name);

    virtual bool decode(const Packet& packet) = 0;

private:
    template<class T>
    static SubtitleDecoder * create() { return new T(); }
    typedef SubtitleDecoder* (*SubtitleDecoderCreator)();

protected:
    static bool Register(SubtitleDecoderId id, SubtitleDecoderCreator, const char *name);
    SubtitleDecoder(SubtitleDecoderPrivate *d);
};

extern SubtitleDecoderId SubtitleDecoderId_FFmpeg;

NAMESPACE_END
#endif //SUBTITLEDECODER_H
