#ifndef ASS_RENDER_H
#define ASS_RENDER_H

extern "C" {
#ifdef SMI_HAVE_LIBASS
#include "ass/ass.h"
#include "ass/ass_types.h"
#endif
#include "libavcodec/avcodec.h"
}
#define UNPREMULTIPLY_ALPHA(x, y) ((((x) << 16) - ((x) << 9) + (x)) / ((((x) + (y)) << 8) - ((x) + (y)) - (y) * (x)))
#define FAST_DIV255(x) ((((x) + 128) * 257) >> 16)

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)
//#define _a(c)  ((0xFF-(c)) &0xFF)

/*
 * ASS_Image: 1bit alpha per pixel + 1 rgb per image. less memory usage
 */
#define ARGB32_SET(C, R, G, B, A) \
    C[3] = (A); \
    C[2] = (R); \
    C[1] = (G); \
    C[0] = (B);
#define ARGB32_ADD(C, R, G, B, A) \
    C[3] += (A); \
    C[2] += (R); \
    C[1] += (G); \
    C[0] += (B);
#define ARGB32_A(C) (C[3])
#define ARGB32_R(C) (C[2])
#define ARGB32_G(C) (C[1])
#define ARGB32_B(C) (C[0])

namespace ASSAide {
class ASSRender
{
public:
    ASSRender() :
#ifdef SMI_HAVE_LIBASS
        library(nullptr),
        renderer(nullptr),
        assTrack(nullptr),
#endif
        width(0),
        height(0),
        init_flag(false)
    {

    }
    ~ASSRender()
    {
#ifdef SMI_HAVE_LIBASS
        if (assTrack)
            ass_free_track(assTrack);
        if (renderer)
            ass_renderer_done(renderer);
        if (library)
            ass_library_done(library);
#endif
    }

    bool initialized() { return init_flag; }

    int initialize(AVCodecContext* ctx, int w, int h)
    {
#ifdef SMI_HAVE_LIBASS
        library = ass_library_init();
        if (!library)
            return AVERROR(EINVAL);
        ass_set_fonts_dir(library, NULL);
        renderer = ass_renderer_init(library);
        if (!renderer)
            return AVERROR(EINVAL);
        assTrack = ass_new_track(library);
        if (!assTrack)
            return AVERROR(EINVAL);
        ass_set_fonts(renderer, NULL, NULL, 1, NULL, 1);
        if (ctx->subtitle_header)
            ass_process_codec_private(assTrack, (char *)ctx->subtitle_header, ctx->subtitle_header_size);
        ass_set_frame_size(renderer, w, h);
        width = w;
        height = h;
        init_flag = true;
#endif
        return 0;
    }

    int addSubtitleToTrack(AVSubtitle* subtitle)
    {
#ifdef SMI_HAVE_LIBASS
        const int64_t start_time = av_rescale_q(subtitle->pts, { 1,AV_TIME_BASE }, { 1,1000 });
        const int64_t duration = subtitle->end_display_time + 1000;
        for (int i = 0; i < subtitle->num_rects; i++)
        {
            char *ass_line = subtitle->rects[i]->ass;
            if (ass_line)
                ass_process_chunk(assTrack, ass_line, strlen(ass_line), start_time, duration);
        }
        int detect_change;
        ASS_Image *image = ass_render_frame(renderer, assTrack, start_time, &detect_change);
        return convertRGBA(width, height, image, subtitle);
#endif
        return 0;
    }

#ifdef SMI_HAVE_LIBASS
    int convertRGBA(int width, int height, ASS_Image* image, AVSubtitle* sub)
    {
        if (!image || !sub || !width || !height)
            return 0;

        int stride = width * 4;

        if (sub->rects[0]->data[0])
            av_freep(&sub->rects[0]->data[0]);
        sub->rects[0]->data[0] = (uint8_t *)av_mallocz(height*stride);
        sub->rects[0]->w = width;
        sub->rects[0]->h = height;
        sub->rects[0]->linesize[0] = stride;
        sub->rects[0]->x = 0;
        sub->rects[0]->y = 0;
        if (!sub->rects[0]->data[0])
            return AVERROR(ENOMEM);

        memset(sub->rects[0]->data[0], 0x00, height * stride);
#if 1
        while (image) {
            const uint8_t a = 255 - _a(image->color);
            if (a == 0) {
                image = image->next;
                continue;
            }
            const uint8_t r = _r(image->color);
            const uint8_t g = _g(image->color);
            const uint8_t b = _b(image->color);
            unsigned char *src = image->bitmap;
            unsigned char *dst = sub->rects[0]->data[0] + image->dst_y * stride + image->dst_x * 4;

            for (int y = 0; y < image->h; ++y) {
                for (int x = 0; x < image->w; ++x) {
                    const unsigned k = ((unsigned)src[x])*a / 255;
                    uint8_t *c = (uint8_t*)(&dst[x * 4]);
                    //test
                    //dst[x * 4 + 0] = 255;
                    //dst[x * 4 + 1] = 255;
                    //dst[x * 4 + 2] = 0;
                    //dst[x * 4 + 3] = 255;
                    //c[0] = 255;
                    //c[1] = 255;
                    //c[2] = 0;
                    //c[3] = 255;
                    //continue;
                    //<-----
                    const unsigned A = ARGB32_A(c);
                    if (A == 0) { // dst color can be ignored
                        ARGB32_SET(c, r, g, b, k);
                    }
                    else if (k == 0) { //no change
                        // do nothing
                    }
                    else if (k == 255) {
                        ARGB32_SET(c, r, g, b, k);
                    }
                    else {
                        const unsigned R = ARGB32_R(c);
                        const unsigned G = ARGB32_G(c);
                        const unsigned B = ARGB32_B(c);
                        ARGB32_ADD(c, r == R ? 0 : k * (r - R) / 255, g == G ? 0 : k * (g - G) / 255, b == B ? 0 : k * (b - B) / 255, a == A ? 0 : k * (a - A) / 255);
                    }
                }
                src += image->stride;
                dst += stride;
            }
            image = image->next;
        }
#else
        while (image) {
            int x, y;
            unsigned char a = _a(image->color);
            unsigned char r = _r(image->color);
            unsigned char g = _g(image->color);
            unsigned char b = _b(image->color);

            unsigned char *src;
            unsigned char *dst;

            src = image->bitmap;

            dst = sub->rects[0]->data[0] + image->dst_y * stride + image->dst_x * 4;
            for (y = 0; y < image->h; ++y) {
                for (x = 0; x < image->w; ++x) {
                    unsigned alpha = ((unsigned)src[x]) * a / 255;
                    unsigned alphasrc = alpha;
                    if (alpha != 0 && alpha != 255)
                        alpha = UNPREMULTIPLY_ALPHA(alpha, dst[x * 4 + 3]);
                    switch (alpha)
                    {
                    case 0:
                        break;
                    case 255:
                        dst[x * 4 + 0] = b;
                        dst[x * 4 + 1] = g;
                        dst[x * 4 + 2] = r;
                        dst[x * 4 + 3] = alpha;
                        break;
                    default:
                        dst[x * 4 + 0] = FAST_DIV255(dst[x * 4 + 0] * (255 - alpha) + b * alpha);
                        dst[x * 4 + 1] = FAST_DIV255(dst[x * 4 + 1] * (255 - alpha) + g * alpha);
                        dst[x * 4 + 2] = FAST_DIV255(dst[x * 4 + 2] * (255 - alpha) + r * alpha);
                        dst[x * 4 + 3] += FAST_DIV255((255 - dst[x * 4 + 3]) * alphasrc);
                    }
                }
                src += image->stride;
                dst += stride;
            }
            image = image->next;
        }
#endif
        return 0;
    }
#endif

private:
#ifdef SMI_HAVE_LIBASS
    ASS_Library* library;
    ASS_Renderer* renderer;
    ASS_Track* assTrack;
#endif
    int width;
    int height;
    bool init_flag;
};
}

#endif
