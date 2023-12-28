#include "VideoDecoder.h"
#include "private/VideoDecoder_p.h"
#include "Factory.h"
#include "mkid.h"

NAMESPACE_BEGIN

FACTORY_DEFINE(VideoDecoder)

VideoDecoderId VideoDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
VideoDecoderId VideoDecoderId_MMAL = mkid::id32base36_6<'F', 'F', 'M', 'M', 'A', 'L'>::value;
VideoDecoderId VideoDecoderId_QSV = mkid::id32base36_5<'F', 'F', 'Q', 'S', 'V'>::value;
VideoDecoderId VideoDecoderId_CrystalHD = mkid::id32base36_5<'F', 'F', 'C', 'H', 'D'>::value;

static void VideoDecoder_RegisterAll()
{
    ONLY_RUN_ONES
    // factory.h does not check whether an id is registered
    if (VideoDecoder::name(VideoDecoderId_FFmpeg)) //registered on load
        return;
    extern bool RegisterVideoDecoderFFmpeg_Man();
    RegisterVideoDecoderFFmpeg_Man();
}

VideoDecoder::VideoDecoder(VideoDecoderPrivate *d):
    AVDecoder(d)
{

}

std::vector<VideoDecoderId> VideoDecoder::registered()
{
    VideoDecoder_RegisterAll();
    return VideoDecoderFactory::instance().registeredIds();
}

StringList VideoDecoder::supportedCodecs()
{
    static StringList codecs;
    if (!codecs.empty())
        return codecs;
    avcodec_register_all();
    AVCodec* c = nullptr;
    while ((c = av_codec_next(c))) {
        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
            continue;
        codecs.push_back(c->name);
    }
    return codecs;
}

std::string VideoDecoder::name() const {
    return std::string();
}

std::string VideoDecoder::description() const {
    return std::string();
}

NAMESPACE_END
