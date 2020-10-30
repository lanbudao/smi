#include "VideoDecoderFFmpegBase.h"
#include "AVDecoder_p.h"
#include "AVLog.h"
#include "inner.h"

//#define VIDEO_DECODER_USE_VIDEO2

NAMESPACE_BEGIN

VideoDecoderFFmpegBase::VideoDecoderFFmpegBase(VideoDecoderFFmpegBasePrivate *d):
    VideoDecoder(d)
{
}

VideoDecoderFFmpegBase::~VideoDecoderFFmpegBase()
{
}

int VideoDecoderFFmpegBase::decode(const Packet &pkt) {
    DPTR_D(VideoDecoderFFmpegBase);

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
    if (ret < 0) {
        AVWarning("Video decoder send packet error: %s.\n", averror2str(ret));
        return ret;
    }
    ret = avcodec_receive_frame(d->codec_ctx, d->frame);
    if (ret >= 0) {
        if (d->decoder_reorder_pts) {
            d->frame->pts = d->frame->best_effort_timestamp;
        } else {
            d->frame->pts = d->frame->pkt_dts;
        }
    }
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            AVWarning("VideoDecoder: %s\n", averror2str(ret));
        } else if (ret == AVERROR(EAGAIN)) {
            AVWarning("Video decoder need new input %s.\n", averror2str(ret));
        } else {
            AVWarning("video decode error: %s\n", averror2str(ret));
        }
    }
    return ret;
}

VideoFrame VideoDecoderFFmpegBase::frame()
{
    DPTR_D(VideoDecoderFFmpegBase);
    if (d->frame->width <= 0 || d->frame->height <= 0 || !d->codec_ctx)
        return VideoFrame();
    VideoFormat format(d->codec_ctx->pix_fmt);
    VideoFrame frame(d->frame->width, d->frame->height, format);
    frame.setDisplayAspectRatio(d->getDisplayAspectRatio(d->frame));
    frame.setDuration(d->getVideoFrameDuration());
//    frame.setBits(d->frame->data);
    frame.setData(d->frame);
//    frame.setBytesPerLine(d->frame->linesize);
//    frame.setTimestamp((double)d->frame->pts / 1000.0);
    d->setColorDetails(frame);
    return frame;
}

NAMESPACE_END
