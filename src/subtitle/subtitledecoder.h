#ifndef SUBTITLE_HANDLER_H
#define SUBTITLE_HANDLER_H

#include "global.h"
#include "DPTR.h"
#include "Factory.h"
#include "Subtitle.h"
NAMESPACE_BEGIN

typedef int SubtitleDecoderId;
class SubtitleDecoderPrivate;
class SubtitleDecoder
{
    FACTORY_INTERFACE(SubtitleDecoder)
    DPTR_DECLARE_PRIVATE(SubtitleDecoder)
public:
    SubtitleDecoder();
    SubtitleDecoder(SubtitleDecoderPrivate* d);
    ~SubtitleDecoder();

    virtual bool processHeader(MediaInfo *info) = 0;
    virtual SubtitleFrame processLine(Packet *pkt) = 0;

private:
    DPTR_DECLARE(SubtitleDecoder)
};

NAMESPACE_END
#endif
