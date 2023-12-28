#ifndef AVDECODER_H
#define AVDECODER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVStream AVStream;

NAMESPACE_BEGIN

class AVDecoderPrivate;
class AVDecoder
{
    DPTR_DECLARE_PRIVATE(AVDecoder)
public:
    virtual ~AVDecoder();

    void initialize(AVFormatContext *ctx, AVStream* stream);
    AVCodecContext * codecCtx();
    const StringList& supportedHardwareCodecs() const;

    virtual std::string name() const = 0;
    virtual std::string description() const = 0;

    void setCodeOptions(std::map<std::string, std::string> &options);

    virtual bool open(const string &extra = string());
    virtual void onOpen() {}
    virtual bool close();

    bool isOpen();
    void flush();

    void setCodecName(const std::string &name);
    std::string codecName() const;

    bool isAvailable() const;

//    void setCodecCtx(void *ctx);
    AVCodec *findCodec(const std::string &name, const std::string &hwaccel, int id);

protected:
    AVDecoder(AVDecoderPrivate *d);
    DPTR_DECLARE(AVDecoder)
};

NAMESPACE_END
#endif //AVDECODER_H
