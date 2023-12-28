#include "subtitledecoder.h"
#include "AVDecoder_p.h"
#include "utils/mkid.h"

NAMESPACE_BEGIN

SubtitleDecoderId SubtitleDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;

FACTORY_DEFINE(SubtitleDecoder);

SubtitleDecoder::SubtitleDecoder(SubtitleDecoderPrivate* d):
    AVDecoder(d)
{

}

SubtitleDecoder::~SubtitleDecoder()
{

}

std::vector<SubtitleDecoderId> SubtitleDecoder::registered()
{
    return SubtitleDecoderFactory::instance().registeredIds();
}

NAMESPACE_END