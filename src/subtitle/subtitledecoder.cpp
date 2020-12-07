#include "subtitledecoder.h"

NAMESPACE_BEGIN

class SubtitleDecoderPrivate
{
public:

};

FACTORY_DEFINE(SubtitleDecoder);
SubtitleDecoder::SubtitleDecoder():
    d_ptr(new SubtitleDecoderPrivate)
{

}

SubtitleDecoder::SubtitleDecoder(SubtitleDecoderPrivate* d):
    d_ptr(d)
{

}

SubtitleDecoder::~SubtitleDecoder()
{

}

NAMESPACE_END