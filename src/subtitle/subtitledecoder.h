#ifndef SUBTITLE_HANDLER_H
#define SUBTITLE_HANDLER_H

#include "global.h"
#include "DPTR.h"
#include "Factory.h"
#include "AVDecoder.h"
#include "Subtitle.h"

NAMESPACE_BEGIN

typedef int SubtitleDecoderId;
class SubtitleDecoderPrivate;
class SubtitleDecoder: public AVDecoder
{
    FACTORY_INTERFACE(SubtitleDecoder)
    DPTR_DECLARE_PRIVATE(SubtitleDecoder)
public:
    ~SubtitleDecoder();

    virtual std::string name() const { return std::string(); }
    virtual std::string description() const { return std::string(); }

    static std::vector<SubtitleDecoderId> registered();
    virtual bool processHeader(MediaInfo *info) = 0;
    virtual SubtitleFrame processLine(Packet *pkt) = 0;
    virtual SubtitleFrame decode(Packet *pkt, int *ret) = 0;

protected:
    SubtitleDecoder(SubtitleDecoderPrivate* d);

};

extern SubtitleDecoderId SubtitleDecoderId_FFmpeg;

NAMESPACE_END
#endif
