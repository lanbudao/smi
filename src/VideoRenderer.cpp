#include "VideoRender_p.h"
#include "renderer/glpackage.h"

#include "glad/glad.h"
#include "innermath.h"

NAMESPACE_BEGIN

VideoRenderer::VideoRenderer():
    AVOutput(new VideoRendererPrivate)
{

}

VideoRenderer::~VideoRenderer()
{

}

void VideoRenderer::setOpaque(void * o)
{
    DPTR_D(VideoRenderer);
    d->opaque = o;
}

void * VideoRenderer::opaque() const
{
    return d_func()->opaque;
}

void VideoRenderer::initVideoRender()
{
	DPTR_D(VideoRenderer);
	d->glv->setMediaInfo(d->media_info);
	d->glv->initGL();
}

void VideoRenderer::setBackgroundColor(const Color &c)
{
	DPTR_D(VideoRenderer);
    d->glv->fill(c);
}

void VideoRenderer::receive(const VideoFrame &frame)
{
	DPTR_D(VideoRenderer);
    std::lock_guard<std::mutex> lock(d->mtx);
#ifdef RENDER_TEST
	d->current_frame = frame;
	d->render_cb(nullptr);
#else
    const float ratio = d->source_aspect_ratio;
	d->source_aspect_ratio = frame.displayAspectRatio();
    if (FuzzyCompare(ratio, d->source_aspect_ratio))
        CALL_BACK(sourceAspectRatioChanged, d->source_aspect_ratio);
	setSourceSize(frame.width(), frame.height());
    d->current_frame = frame;
    d->prepareExternalSubtitleFrame();
    update();
#endif
}

void VideoRenderer::receiveSubtitle(SubtitleFrame & frame)
{
    DPTR_D(VideoRenderer);
    d->current_sub_frame = frame;
}

void VideoRenderer::setSourceSize(int width, int height)
{
	DPTR_D(VideoRenderer);
	if (d->src_width != width || d->src_height != height) {
		d->aspect_ratio_changed = true; //?? for VideoAspectRatio mode
		d->src_width = width;
		d->src_height = height;
	}
	if (!d->aspect_ratio_changed)// && (d->src_width == width && d->src_height == height))
		return;
	//d->source_aspect_ratio = qreal(d->src_width)/qreal(d->src_height);
	//tDebug("%s => calculating aspect ratio from converted input data(%f)", __FUNCTION__, d->source_aspect_ratio);
	//see setOutAspectRatioMode
	if (d->out_aspect_ratio_mode == VideoAspectRatio) {
		//source_aspect_ratio equals to original video aspect ratio here, also equals to out ratio
		setOutAspectRatio(d->source_aspect_ratio);
	}
	d->aspect_ratio_changed = false; //TODO: why graphicsitemrenderer need this? otherwise aspect_ratio_changed is always true?
}

void VideoRenderer::resizeWindow(int width, int height)
{
	DPTR_D(VideoRenderer);
	if (width == 0 || height == 0 || (d->renderer_width == width && d->renderer_height == height))
		return;
    d->renderer_width = width;
    d->renderer_height = height;
	if (d->computeOutParameters(d->out_aspect_ratio)) {
		// videoRectChanged();
		// contentRectChanged();
	}
    onResizeWindow();
}

void VideoRenderer::renderVideo()
{
	DPTR_D(VideoRenderer);
    std::lock_guard<std::mutex> lock(d->mtx);
#ifdef RENDER_TEST
    d->glv->renderVideo(d->current_frame);
#else
	RectF roi = realROI();
	//d->glv.render(QRectF(-1, 1, 2, -2), roi, d->matrix);
    if (d->frame_changed) {
        d->applyVideoFilter();
		d->glv->setCurrentFrame(d->current_frame);
		d->frame_changed = false;
    }
    d->glv->render(RectF(), roi, d->matrix);
//    fprintf(stderr, "xxxx: %.3f %.3f\n", d->current_frame.timestamp(), d->current_frame.ptsOfPkt());
//    fflush(stderr);
    d->applyRenderFilter();

    // render subtitle frame
    d->renderSubtitleFrame();
#endif

}

RectF VideoRenderer::realROI() const
{
	DPTR_D(const VideoRenderer);
	if (!d->roi.isValid()) {
		return RectF(PointF(), d->current_frame.size().toSizeF());
	}
	RectF r = d->roi;
	// nomalized x, y < 1
	bool normalized = false;
	if (std::abs(d->roi.x()) < 1) {
		normalized = true;
		r.setX(d->roi.x()*double(d->src_width)); //TODO: why not video_frame.size()? roi not correct
	}
	if (std::abs(d->roi.y()) < 1) {
		normalized = true;
		r.setY(d->roi.y()*double(d->src_height));
	}
	// whole size use width or height = 0, i.e. null size
	// nomalized width, height <= 1. If 1 is normalized value iff |x|<1 || |y| < 1
	if (std::abs(d->roi.width()) < 1)
		r.setWidth(d->roi.width()*double(d->src_width));
	if (std::abs(d->roi.height()) < 1)
		r.setHeight(d->roi.height()*double(d->src_height));
	if (d->roi.width() == 1.0 && normalized) {
		r.setWidth(d->src_width);
	}
	if (d->roi.height() == 1.0 && normalized) {
		r.setHeight(d->src_height);
	}
	//TODO: insect with source rect?
	return r;
}

void VideoRenderer::onResizeWindow()
{
	DPTR_D(VideoRenderer);
	d->glv->setViewPort(RectF(0, 0, d->renderer_width, d->renderer_height));
	setupAspectRatio();
    update();
}

void VideoRenderer::update()
{
    DPTR_D(VideoRenderer);
    d->frame_changed = true;
    CALL_BACK(d->render_cb, d->opaque);
}

void VideoRenderer::setOutAspectRatio(double ratio)
{
	DPTR_D(VideoRenderer);
	bool ratio_changed = d->out_aspect_ratio != ratio;
	d->out_aspect_ratio = ratio;
	//indicate that this function is called by user. otherwise, called in VideoRenderer
	if (!d->aspect_ratio_changed) {
		if (d->out_aspect_ratio_mode != CustomAspectRation) {
			d->out_aspect_ratio_mode = CustomAspectRation;
			//Q_EMIT outAspectRatioModeChanged();
		}
	}
	d->aspect_ratio_changed = false; //TODO: when is false?
	if (d->out_aspect_ratio_mode != RendererAspectRatio) {
		d->update_background = true; //can not fill the whole renderer with video
	}
	//compute the out out_rect
	if (d->computeOutParameters(ratio)) {
		//Q_EMIT videoRectChanged();
		//Q_EMIT contentRectChanged();
	}
	if (ratio_changed) {
		//onSetOutAspectRatio(ratio);
		//Q_EMIT outAspectRatioChanged();
	}
	//update
	setupAspectRatio();
}

void VideoRenderer::setupAspectRatio()
{
	DPTR_D(VideoRenderer);
	d->matrix.setToIdentity();
	d->matrix.scale((GLfloat)d->out_rect.width() / (GLfloat)d->renderer_width, 
		(GLfloat)d->out_rect.height() / (GLfloat)d->renderer_height, 1);
	if (d->rotation())
		d->matrix.rotate(d->rotation(), 0, 0, 1); // Z axis
}

void VideoRenderer::setInternalSubtitleEnabled(bool enabled)
{
    DPTR_D(VideoRenderer);
    d->internal_subtitle_enabled = enabled;
}

void VideoRenderer::addSubtitle(Subtitle * subtitle)
{
    DPTR_D(VideoRenderer);
    d->subtitles.push_back(subtitle);
}

void VideoRenderer::updateFilters(const std::list<Filter*> filters)
{
    DPTR_D(VideoRenderer);
    std::unique_lock<std::mutex> lock_mtx(d->mtx);
    d->filters = filters;
}

const std::list<Filter*>& VideoRenderer::filters() const
{
    return d_func()->filters;
}

NAMESPACE_END
