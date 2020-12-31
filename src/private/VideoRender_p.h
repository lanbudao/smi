#ifndef VIDEORENDERER_P_H
#define VIDEORENDERER_P_H

#include "VideoRenderer.h"
#include "AVOutput_p.h"
#include "renderer/OpenglVideo.h"
#include "sdk/filter/filter.h"
#include "subtitle.h"
#include "subtitle/subtitleframe.h"
#include <cmath>

NAMESPACE_BEGIN

#define PIXMAP_VERTEX_POS 10
#define PIXMAP_TEXTURE_POS 11

static const char* quad_shader_vs = GET_STR(
    attribute vec2 in_position;
    attribute vec2 in_tex_coord;
    out vec2 tex_coord;
    void main(void)
    {
        gl_Position = vec4(in_position, 0.0, 1.0);
        tex_coord = in_tex_coord;
    }
);

static const char* quad_shader_fs = GET_STR(
    in vec2 tex_coord;
    uniform sampler2D bitmap_tex;
    void main(void)
    {
        gl_FragColor = texture2D(bitmap_tex, tex_coord);
    }
);

class PixmapRender
{
public:
    PixmapRender() :
        program(nullptr),
        vao(nullptr),
        bufVer(nullptr),
        bufTex(nullptr)
    {
    }
    void initialize();
    void render(void *data, int w, int h);

private:
    GLShaderProgram* program;
    GLArray *vao;
    GLBuffer *bufVer, *bufTex;
    int unis = 0;
    GLuint texs = 0;
};

void PixmapRender::initialize()
{
    program = new GLShaderProgram();
    vao = new GLArray();
    vao->create();

    static const GLfloat pix_ver[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
    };
    static const GLfloat pix_tex[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };
    vao->bind();
    bufVer = new GLBuffer(GLBuffer::VertexBuffer);
    bufVer->create();
    bufVer->bind();
    bufVer->allocate(pix_ver, sizeof(pix_ver));
    glVertexAttribPointer(PIXMAP_VERTEX_POS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid *)0);
    glEnableVertexAttribArray(PIXMAP_VERTEX_POS);
    vao->unbind();

    vao->bind();
    bufTex = new GLBuffer(GLBuffer::VertexBuffer);
    bufTex->create();
    bufTex->bind();
    bufTex->allocate(pix_tex, sizeof(pix_tex));
    glVertexAttribPointer(PIXMAP_TEXTURE_POS, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid *)0);
    glEnableVertexAttribArray(PIXMAP_TEXTURE_POS);
    vao->unbind();

    program->addShaderFromSourceCode(GLShaderProgram::Vertex, quad_shader_vs);
    program->addShaderFromSourceCode(GLShaderProgram::Fragment, quad_shader_fs);
    program->bindAttributeLocation("in_position", PIXMAP_VERTEX_POS);
    program->bindAttributeLocation("in_tex_coord", PIXMAP_TEXTURE_POS);
    program->link();
    program->bind();

    unis = program->uniformLocation("bitmap_tex");

    glGenTextures(1, &texs);
    glBindTexture(GL_TEXTURE_2D, texs);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void PixmapRender::render(void *data, int w, int h)
{
    if (!program || !program->isValid())
        return;
    program->bind();
    vao->bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texs);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    glUniform1i(unis, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao->unbind();
    program->unBind();
}

class VideoRendererPrivate: public AVOutputPrivate
{
public:
    VideoRendererPrivate():
        opaque(nullptr),
        renderer_width(0),
        renderer_height(0),
		glv(nullptr),
		frame_changed(false),
		update_background(true),
		aspect_ratio_changed(true),
		source_aspect_ratio(1.0),
		out_aspect_ratio_mode(VideoRenderer::VideoAspectRatio),
		out_aspect_ratio(0),
		orientation(0),
        internal_subtitle_enabled(true)
    {
		glv = new OpenglVideo();
        pixmap_render.initialize();
    }

    ~VideoRendererPrivate()
    {
		delete glv;
	}
	int rotation() const;

	bool computeOutParameters(double outAspectRatio);

    void applyVideoFilter();
    void applyRenderFilter();
    void processSubtitleSeek();
    void prepareExternalSubtitleFrame();
    void renderSubtitleFrame();

    void* opaque;
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

    std::list<Filter*> filters;

    std::list<Subtitle*> subtitles;
    bool internal_subtitle_enabled;
    SubtitleFrame current_sub_frame;
    PixmapRender pixmap_render;
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

inline void VideoRendererPrivate::applyVideoFilter()
{
    if (!filters.empty()) {
        for (std::list<Filter*>::iterator it = filters.begin();
            it != filters.end(); ++it) {
            if ((*it)->type() != Filter::Video)
                continue;
            VideoFilter *f = static_cast<VideoFilter*>(*it);
            if (!f->enabled())
                continue;
            f->apply(media_info, &current_frame);
        }
    }
}

inline void VideoRendererPrivate::applyRenderFilter()
{
    if (!filters.empty()) {
        for (std::list<Filter*>::iterator it = filters.begin();
            it != filters.end(); ++it) {
            if ((*it)->type() != Filter::Render)
                continue;
            RenderFilter *f = static_cast<RenderFilter*>(*it);
            if (!f->enabled())
                continue;
            f->apply(media_info, &current_frame);
        }
    }
}

void VideoRendererPrivate::processSubtitleSeek()
{
    std::list<Subtitle*>::iterator it = subtitles.begin();
    for (it; it != subtitles.end(); ++it) {
        Subtitle* subtitle = *it;
        if (!subtitle)
            continue;
        subtitle->processSeek();
    }
}

void VideoRendererPrivate::prepareExternalSubtitleFrame()
{
    std::list<Subtitle*>::iterator it = subtitles.begin();
    for (it; it != subtitles.end(); ++it) {
        Subtitle* subtitle = *it;
        if (!subtitle || !subtitle->enabled())
            continue;
        /* prepare subtitle frame */
        subtitle->setTimestamp(current_frame.timestamp(), current_frame.width(), current_frame.height());
    }
}

void VideoRendererPrivate::renderSubtitleFrame()
{
    /* internal subtitle*/
    if (internal_subtitle_enabled && current_sub_frame.valid()) {
        AVSubtitle *data = current_sub_frame.data();
        double vpts = current_frame.timestamp();
        if (vpts < current_sub_frame.start || vpts > current_sub_frame.end)
            return;
        if (data->num_rects > 0 && data->rects[0] && data->rects[0]->data[0]) {
            pixmap_render.render(data->rects[0]->data[0], current_frame.width(), current_frame.height());
        }
    }
    /* external subtitles */
    std::list<Subtitle*>::iterator it = subtitles.begin();
    for (it; it != subtitles.end(); ++it) {
        Subtitle* subtitle = *it;
        if (!subtitle || !subtitle->enabled())
            continue;
        SubtitleFrame* frame = subtitle->frame();
        if (frame->valid()) {
            AVSubtitle *data = frame->data();
            double vpts = current_frame.timestamp();
            if (vpts < frame->start || vpts > frame->end)
                continue;
            if (data->num_rects > 0 && data->rects[0] && data->rects[0]->data[0]) {
                pixmap_render.render(data->rects[0]->data[0], current_frame.width(), current_frame.height());
            }
        }
    }
}

NAMESPACE_END
#endif //AVOUTPUT_P_H
