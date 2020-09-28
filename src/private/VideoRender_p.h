#ifndef VIDEORENDERER_P_H
#define VIDEORENDERER_P_H

#include "VideoRenderer.h"
#include "AVOutput_p.h"
#include "renderer/OpenglVideo.h"
#include <cmath>

NAMESPACE_BEGIN
class VideoRendererPrivate: public AVOutputPrivate
{
public:
    VideoRendererPrivate():
        renderer_width(0),
        renderer_height(0),
		glv(nullptr),
		frame_changed(false),
		update_background(true),
		aspect_ratio_changed(true),
		source_aspect_ratio(1.0),
		out_aspect_ratio_mode(VideoRenderer::VideoAspectRatio),
		out_aspect_ratio(0),
		orientation(0)
    {
		glv = new OpenglVideo();
    }

    ~VideoRendererPrivate()
    {
		delete glv;
	}
	int rotation() const;

	bool computeOutParameters(double outAspectRatio);

    int renderer_width, renderer_height;
    VideoRenderer::OutAspectRatioMode out_ratio_mode;
	OpenglVideo *glv;
	Matrix4x4 matrix;
	bool frame_changed;
	int src_width, src_height; //TODO: in_xxx
	bool update_background;
	bool aspect_ratio_changed;
	VideoRenderer::OutAspectRatioMode out_aspect_ratio_mode;
	double out_aspect_ratio;
    float source_aspect_ratio;
	RectF out_rect;
	RectF roi;
	
	int orientation;
};

int VideoRendererPrivate::rotation() const {
    if (media_info->video) {
        return FORCE_INT(media_info->video->rotate) + orientation;
	}
	return orientation;
}

bool VideoRendererPrivate::computeOutParameters(double outAspectRatio) 
{
	double rendererAspectRatio = double(renderer_width) / double(renderer_height);
	const RectF out_rect0(out_rect);
	if (out_aspect_ratio_mode == VideoRenderer::RendererAspectRatio) {
		out_aspect_ratio = rendererAspectRatio;
		out_rect = RectF(0, 0, renderer_width, renderer_height);
		return out_rect0 != out_rect;
	}
	// dar: displayed aspect ratio in video renderer orientation
    int rotate = orientation;
    if (media_info->video) {
        rotate += FORCE_INT(media_info->video->rotate);
	}
	const double dar = (rotate % 180) ? 1.0 / outAspectRatio : outAspectRatio;
	//qDebug("out rect: %f %dx%d ==>", out_aspect_ratio, out_rect.width(), out_rect.height());
	if (rendererAspectRatio >= dar) { //equals to original video aspect ratio here, also equals to out ratio
		//renderer is too wide, use renderer's height, horizonal align center
		const int h = renderer_height;
		const int w = std::round(dar * double(h));
		out_rect = RectF((renderer_width - w) / 2, 0, w, h);
	}
	else if (rendererAspectRatio < dar) {
		//renderer is too high, use renderer's width
		const int w = renderer_width;
		const int h = std::round(double(w) / dar);
		out_rect = RectF(0, (renderer_height - h) / 2, w, h);
	}
	out_aspect_ratio = outAspectRatio;
	//qDebug("%f %dx%d <<<<<<<<", out_aspect_ratio, out_rect.width(), out_rect.height());
	return out_rect0 != out_rect;
}

NAMESPACE_END
#endif //AVOUTPUT_P_H
