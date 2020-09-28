#include "SubtitleDecoder.h"
#include "AVDecoder_p.h"
#include "Factory.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(SubtitleDecoder)

static void SubtitleDecoder_RegisterAll()
{
    ONLY_RUN_ONES;
    if (SubtitleDecoder::name(SubtitleDecoderId_FFmpeg)) {
        return;
    }
    extern bool RegisterSubtitleDecoderFFmpeg_Man();
    RegisterSubtitleDecoderFFmpeg_Man();
}

SubtitleDecoder::SubtitleDecoder(SubtitleDecoderPrivate *d):
    AVDecoder(d)
{

}

SubtitleDecoder::~SubtitleDecoder()
{

}

std::string SubtitleDecoder::name() const {
    return std::string();
}

std::string SubtitleDecoder::description() const {
    return std::string();
}

NAMESPACE_END
