#ifndef VIDEORENDERER_P_H
#define VIDEORENDERER_P_H

#include "VideoRenderer.h"
#include "AVOutput_p.h"
#include "renderer/OpenglVideo.h"
#include "sdk/filter/filter.h"
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

    void readFile();

    void initialize();

    void renderPixmap(void *data);

private:
    GLShaderProgram* program;
    GLArray *vao;
    GLBuffer *bufVer, *bufTex;
    //shader中的yuv变量地址
    int unis = 0;
    //opengl中的材质地址
    GLuint texs = 0;
    //材质内存空间
    unsigned char *datas = nullptr;

    GLint imagewidth;
    GLint imageheight;
    GLint pixellength;
    GLubyte* pixeldata;
};

void PixmapRender::readFile()
{
    //打开文件
    FILE* pfile = fopen("E:/welcome_en.bmp", "rb");
    if (pfile == 0) exit(0);
    //读取图像大小
    fseek(pfile, 0x0012, SEEK_SET);
    fread(&imagewidth, sizeof(imagewidth), 1, pfile);
    fread(&imageheight, sizeof(imageheight), 1, pfile);
    //计算像素数据长度
    pixellength = imagewidth * 3;
    while (pixellength % 4 != 0)pixellength++;
    pixellength *= imageheight;
    //读取像素数据
    pixeldata = (GLubyte*)malloc(pixellength);
    if (pixeldata == 0) exit(0);
    fseek(pfile, 54, SEEK_SET);
    fread(pixeldata, pixellength, 1, pfile);
    //关闭文件
    fclose(pfile);

    //int i, j = 0; //Index variables
    //FILE *file;//File pointer
    ////unsigned char *l_texture; //The pointer to the memory zone in which we will load thetexture
    //// windows.hgives us these types to work with the Bitmap files
    //BITMAPFILEHEADER fileheader;
    //BITMAPINFOHEADER infoheader;
    //RGBTRIPLE rgb;
    //if ((file = fopen("E:/welcome_en.bmp", "rb")) == NULL)
    //    return;// Open the file for reading   
    //fread(&fileheader, sizeof(fileheader), 1, file);// Read the fileheader
    //fseek(file, sizeof(fileheader), SEEK_SET);// Jump the fileheader
    //fread(&infoheader, sizeof(infoheader), 1, file);// and read the infoheader
    //// Now we need toallocate the memory for our image (width * height * color deep)
    //pixeldata = (uchar *)malloc(infoheader.biWidth*infoheader.biHeight * 4);
    //// And fill itwith zeros
    //memset(pixeldata, 0, infoheader.biWidth *infoheader.biHeight * 4);
    //// At this pointwe can read every pixel of the image
    //for (i = 0; i < infoheader.biWidth*infoheader.biHeight; i++)
    //{
    //    // Weload an RGB value from the file
    //    fread(&rgb, sizeof(rgb), 1, file);
    //    // Andstore it
    //    pixeldata[j + 0] = rgb.rgbtRed;// Redcomponent
    //    pixeldata[j + 1] = rgb.rgbtGreen;// Greencomponent
    //    pixeldata[j + 2] = rgb.rgbtBlue;// Bluecomponent
    //    pixeldata[j + 3] = 255;// Alphavalue
    //    j += 4; // Go to the next position
    //}
    //imagewidth = infoheader.biWidth;
    //imageheight = infoheader.biHeight;
    //fclose(file);// Close
}

void PixmapRender::initialize()
{
    //readFile();
    //return;
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

    //从shader中获取材质
    unis = program->uniformLocation("bitmap_tex");


    //创建材质
    glGenTextures(1, &texs);
    glBindTexture(GL_TEXTURE_2D, texs);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //创建材质显卡空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, 1280, 720, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imagewidth, imageheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata);

    //分配材质内存空间
    //datas = new unsigned char[imagewidth * imageheight];       //y
}

void PixmapRender::renderPixmap(void *data)
{
    //glRasterPos2f(-1.0f, 0.0f);
    //glPixelZoom(0.5f, 0.5f);
    //glDrawPixels(imagewidth, imageheight, GL_RGBA, GL_UNSIGNED_BYTE, data);
    //return;
    if (!program || !program->isValid())
        return;
    program->bind();
    vao->bind();

    glDepthMask(GL_FALSE);//关掉深度测试
    glEnable(GL_BLEND); //开混合模式贴图
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// 指定混合模式算法

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texs);  //0层绑定到y材质
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //copy the data to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, 1280, 720, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
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
		orientation(0)
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

    void parseAssSubtitle(SubtitleFrame &frame);
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

void VideoRendererPrivate::parseAssSubtitle(SubtitleFrame & frame)
{
    //for (int i = 0; i < data->num_rects; ++i) {
    //    // TODO

    //}
}

void VideoRendererPrivate::renderSubtitleFrame()
{
    AVSubtitle *data = current_sub_frame.data();
    double vpts = current_frame.timestamp();
    if (vpts < current_sub_frame.start || vpts > current_sub_frame.end)
        return;
    //glRasterPos2f(-1.0f, -1.0f);
    //glPixelZoom(0.1f, -0.1f);
    for (int i = 0; i < data->num_rects; ++i) {
        if (data->rects[i] && data->rects[i]->data[0]) {
            //glDrawPixels(data->rects[i]->w, data->rects[i]->h, GL_BGR, GL_UNSIGNED_BYTE, data->rects[i]->data);
            //glDrawPixels(800, 100, GL_RGBA, GL_UNSIGNED_BYTE, data->rects[i]->data[0]);
            pixmap_render.renderPixmap(data->rects[0]->data[0]);
        }
    }
    //}
    //if (data->num_rects > 0 && data->rects[0] && data->rects[0]->data[0])
    //    glDrawPixels(data->rects[0]->w, data->rects[0]->h,
    //        GL_RGBA, GL_UNSIGNED_BYTE, data->rects[0]->data[0]);
}

NAMESPACE_END
#endif //AVOUTPUT_P_H
