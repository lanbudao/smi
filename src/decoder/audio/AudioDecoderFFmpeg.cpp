#include "AudioDecoder.h"
#include "Factory.h"
#include "AVDecoder_p.h"
#include "inner.h"
#include "sdk/DPTR.h"
#include "AVLog.h"
#include "mkid.h"
#include "util/innermath.h"

NAMESPACE_BEGIN

class AudioDecoderFFmpegPrivate: public AudioDecoderPrivate
{
public:
    AudioDecoderFFmpegPrivate():
        frame(nullptr)
    {
        frame = av_frame_alloc();
    }
    virtual ~AudioDecoderFFmpegPrivate()
    {
//        if (frame) {
//            av_frame_free(&frame);
//            frame = nullptr;
//        }
    }

    AVFrame *frame;
};

class AudioDecoderFFmpeg: public AudioDecoder
{
    DPTR_DECLARE_PRIVATE(AudioDecoderFFmpeg)
public:
    AudioDecoderFFmpeg();
    ~AudioDecoderFFmpeg();

    AudioDecoderId id() const;
    std::string description() const;

    AudioFrame frame();

    int decode(const Packet& packet);
};

AudioDecoderId AudioDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER(AudioDecoder, FFmpeg, "FFmpeg")

AudioDecoderFFmpeg::AudioDecoderFFmpeg():
    AudioDecoder(new AudioDecoderFFmpegPrivate)
{

}

AudioDecoderFFmpeg::~AudioDecoderFFmpeg()
{

}

AudioDecoderId AudioDecoderFFmpeg::id() const
{
    return AudioDecoderId_FFmpeg;
}

std::string AudioDecoderFFmpeg::description() const
{
    const int patch = FFPRO_VERSION_PATCH((int)avcodec_version());
    return "";
//    return Util::sformat("%s avcodec %d.%d.%d",
//            (patch >= 100 ? "FFmpeg" : "Libav"),
//            (FFPRO_VERSION_MAJOR((int)avcodec_version())),
//            (FFPRO_VERSION_MINOR((int)avcodec_version())),
//            patch);
}

AudioFrame AudioDecoderFFmpeg::frame()
{
    DPTR_D(AudioDecoderFFmpeg);

    AudioFrame f;
    f.setData(d->frame);
    return f;
}

int AudioDecoderFFmpeg::decode(const Packet &pkt)
{
    DPTR_D(AudioDecoderFFmpeg);
    if (!isAvailable())
        return false;
    int ret = 0;

    if (pkt.isEOF()) {
        AVPacket eofpkt;
        av_init_packet(&eofpkt);
        eofpkt.data = nullptr;
        eofpkt.size = 0;
        ret = avcodec_send_packet(d->codec_ctx, &eofpkt);
        av_packet_unref(&eofpkt);
    } else {
        ret = avcodec_send_packet(d->codec_ctx, pkt.asAVPacket());
    }

    ret = avcodec_receive_frame(d->codec_ctx, d->frame);

    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            AVWarning() << "End of file, " << averror2str(ret);
        } else if (ret == AVERROR(EAGAIN)) {
            AVWarning("Audio decoder need new input %s.\n", averror2str(ret));
        } else {
            AVWarning("Audio decode error: %s\n", averror2str(ret));
        }
    }
    return ret;
}

NAMESPACE_END
