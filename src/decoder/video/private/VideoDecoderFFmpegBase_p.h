#ifndef VIDEODECODERFFMPEGBASE_P_H
#define VIDEODECODERFFMPEGBASE_P_H

#include "VideoDecoder_p.h"

NAMESPACE_BEGIN

extern ColorSpace colorSpaceFromFFmpeg(AVColorSpace space);
extern ColorRange colorRangeFromFFmpeg(AVColorRange range);

class VideoDecoderFFmpegBasePrivate: public VideoDecoderPrivate
{
public:
    VideoDecoderFFmpegBasePrivate()
    {
    }
    virtual ~VideoDecoderFFmpegBasePrivate()
    {
    }

    void setColorDetails(VideoFrame &f)
    {
        if (f.pixelFormat() == frame->format) {
            setColorDetailFFmpeg(f);
            return;
        }
        // hw decoder output frame may have a different format, e.g. gl interop frame may have rgb format for rendering(stored as yuv)
        bool isRGB = f.format().isRGB();
        if (isRGB) {
            f.setColorSpace(f.format().isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
            f.setColorRange(ColorRange_Full);
            return;
        }
        uint64_t flags = av_pix_fmt_desc_get((AVPixelFormat)f.pixelFormatFFmpeg())->flags;
        bool isRgbCoded = ((flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB);
        if (isRgbCoded) {
            if (f.width() >= 1280 && f.height() >= 576) {
                f.setColorSpace(ColorSpace_BT709);
            } else {
                f.setColorSpace(ColorSpace_BT601);
            }
        } else {
            setColorDetailFFmpeg(f);
        }
    }

    void setColorDetailFFmpeg(VideoFrame &f)
    {
        ColorSpace space = colorSpaceFromFFmpeg(av_frame_get_colorspace(frame));
        if (space == ColorSpace_Unknown)
            space = colorSpaceFromFFmpeg(codec_ctx->colorspace);
        f.setColorSpace(space);

        ColorRange range = colorRangeFromFFmpeg(av_frame_get_color_range(frame));
        if (range == ColorRange_Unknown) {
            AVPixelFormat pixfmt_ff = (AVPixelFormat)f.pixelFormatFFmpeg();
            switch (pixfmt_ff) {
                case AV_PIX_FMT_YUVJ420P:
                case AV_PIX_FMT_YUVJ422P:
                case AV_PIX_FMT_YUVJ440P:
                case AV_PIX_FMT_YUVJ444P:
                    range = ColorRange_Full;
                    break;
                default:
                    break;
            }
        }
        if (range == ColorRange_Unknown) {
            range = colorRangeFromFFmpeg(codec_ctx->color_range);
            if (range == ColorRange_Unknown) {
                if (f.format().isXYZ()) {
                    range = ColorRange_Full;
                } else if (!f.format().isRGB()) {
                    range = ColorRange_Limited;
                }
            }
        }
        f.setColorRange(range);
    }

    float getDisplayAspectRatio(AVFrame *f)
    {
        // lavf 54.5.100 av_guess_sample_aspect_ratio: stream.sar > frame.sar
        float dar = 0;
        if (f->height > 0)
            dar = static_cast<float>(f->width) / static_cast<float>(f->height);
        // prefer sar from AVFrame if sar != 1/1
        if (f->sample_aspect_ratio.num > 1)
            dar *= static_cast<float>(av_q2d(f->sample_aspect_ratio));
        else if (codec_ctx && codec_ctx->sample_aspect_ratio.num > 1) // skip 1/1
            dar *= static_cast<float>(av_q2d(codec_ctx->sample_aspect_ratio));
        return dar;
    }

    double getVideoFrameDuration()
    {
        AVRational frame_rate = av_guess_frame_rate(fmt_ctx, current_stream, nullptr);
        AVRational d = {frame_rate.den, frame_rate.num};
        return (frame_rate.num && frame_rate.den ? av_q2d(d) : 0);
    }

};

NAMESPACE_END

#endif //VIDEODECODERFFMPEGBASE_P_H
