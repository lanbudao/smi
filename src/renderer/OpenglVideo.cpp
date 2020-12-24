#include "OpenglVideo.h"
#include "ShaderManager.h"
#include "VideoMaterial.h"
#include "VideoShader.h"
#include "GeometryRenderer.h"
#include "OpenglAide.h"
#include "AVLog.h"
#include "subtitlerender.h"
//#include <GL/GLU.h>
//#define TEST_YUV
#include "glpackage.h"


//传递顶点和材质坐标
static const GLfloat vertexandtexture[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f
};

static const GLfloat ver[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f,
};
static const GLfloat tex[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f
};

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
        gl_FragColor = /*vec4(1.0, 1.0, 0.0, 1.0)*/texture2D(bitmap_tex, tex_coord);
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

    void renderPixmap();

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
}

void PixmapRender::initialize()
{
    readFile();
    //return;
    program = new GLShaderProgram();
    vao = new GLArray();
    vao->create();

    static const GLfloat pix_ver[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f,
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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //创建材质显卡空间
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, imagewidth, imageheight, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imagewidth, imageheight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixeldata);

    //分配材质内存空间
    //datas = new unsigned char[imagewidth * imageheight];       //y
}

void PixmapRender::renderPixmap()
{
    //glRasterPos2f(-1.0f, -1.0f);
    //glPixelZoom(0.5f, 0.5f);
    //glDrawPixels(imagewidth, imageheight, GL_BGR, GL_UNSIGNED_BYTE, pixeldata);
    //return;
    if (!program || !program->isValid())
        return;
    program->bind();
    vao->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texs);  //0层绑定到y材质
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //copy the data to texture
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_BGR, imagewidth, imageheight, 0, GL_BGR, GL_UNSIGNED_BYTE, pixeldata);
    glUniform1i(unis, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    vao->unbind();
    program->unBind();
}

class OpenglVideoPrivate
{
public:
	OpenglVideoPrivate():
		manager(nullptr),
		material(nullptr),
		norm_viewport(true),
		update_geo(true),
		tex_target(0),
		valiad_tex_width(1.0),
		mesh_type(OpenglVideo::RectMesh),
		geometry(nullptr),
		gr(nullptr),
		user_shader(nullptr)
	{
        sub_render.setFontFile("E:/ht.ttf");
	}
	~OpenglVideoPrivate()
	{

	}

	void resetGL();
	void updateGeometry(VideoShader* shader, const RectF &t, const RectF &r);

	ShaderManager *manager;
	VideoMaterial *material;
	long long material_type;
	bool norm_viewport;
	bool has_a;
	bool update_geo;
	int tex_target;
	Size video_size;
	RectF target;
	RectF roi; //including invalid padding width
	OpenglVideo::MeshType mesh_type;
	double valiad_tex_width;
	RectF rect;
	Matrix4x4 matrix;
	TexturedGeometry *geometry;
	GeometryRenderer* gr;
	VideoShader *user_shader;
    Color background;
    SubtitleRender sub_render;
    PixmapRender pixmap_render;
};

void OpenglVideoPrivate::resetGL() {
	if (gr)
		gr->updateGeometry(nullptr);
	if (!manager)
		return;
	delete manager;
	manager = 0;
	if (material) {
		delete material;
		material = 0;
	}
}

void OpenglVideoPrivate::updateGeometry(VideoShader* shader, const RectF &t, const RectF &r)
{
	// also check size change for normalizedROI computation if roi is not normalized
	const bool roi_changed = valiad_tex_width != material->validTextureWidth() || roi != r || video_size != material->frameSize();
	const int tc = shader->textureLocationCount();
	if (roi_changed) {
		roi = r;
		valiad_tex_width = material->validTextureWidth();
		video_size = material->frameSize();
	}
	if (tex_target != shader->textureTarget()) {
		tex_target = shader->textureTarget();
		update_geo = true;
	}
	static bool update_gr = false;
	if (!gr || update_gr) { // TODO: only update VAO, not the whole GeometryRenderer
		update_gr = false;
		update_geo = true;
		GeometryRenderer *r = new GeometryRenderer(); // local var is captured by lambda 
		gr = r;
	}
	// (-1, -1, 2, 2) must flip y
	RectF target_rect = norm_viewport ? RectF(-1, 1, 2, -2) : rect;
	if (target.isValid()) {
		if (roi_changed || target != t) {
			target = t;
			update_geo = true;
			//target_rect = target (if valid). // relate to gvf bug?
		}
	}
	else {
		if (roi_changed) {
			update_geo = true;
		}
	}
	if (!update_geo)
		return;
	delete geometry;
	geometry = NULL;
	if (mesh_type == OpenglVideo::SphereMesh)
		geometry = new Sphere();
	else
		geometry = new TexturedGeometry();
	//qDebug("updating geometry...");
	// setTextureCount may change the vertex data. Call it before setRect()
	AVDebug("target rect: %f, %f, %f, %f.\n" ,target_rect.x(), target_rect.y(), target_rect.width(), target_rect.height());
	geometry->setTextureCount(shader->textureTarget() == GL_TEXTURE_RECTANGLE ? tc : 1);
	geometry->setGeometryRect(target_rect);
	geometry->setTextureRect(material->mapToTexture(0, roi));
	if (shader->textureTarget() == GL_TEXTURE_RECTANGLE) {
		for (int i = 1; i < tc; ++i) {
			// tc can > planes, but that will compute chroma plane
			geometry->setTextureRect(material->mapToTexture(i, roi), i);
		}
	}
	geometry->create();
	update_geo = false;
	gr->updateGeometry(geometry);
}

OpenglVideo::OpenglVideo():
	d_ptr(new OpenglVideoPrivate)
{
    if (!gladLoadGL()) {
        AVError("glad load GL failed!\n");
        return;
    }
#ifdef RENDER_TEST
	media_info = nullptr;
	program = nullptr;
	vao = nullptr;
	fill(Color(0, 255, 255, 255));
#else
	initialize();
#endif
}

OpenglVideo::~OpenglVideo()
{
#ifdef RENDER_TEST
	delete datas[0];
	delete datas[1];
	delete datas[2];
#endif
}

void OpenglVideo::setMediaInfo(MediaInfo *info)
{
#ifdef RENDER_TEST
	media_info = info;
#endif
}

void OpenglVideo::initGL()
{
#ifdef RENDER_TEST
	GLint ret;

#ifdef TEST_YUV
	int width = 400, height = 240;
#else
	//if (media_info->video)
	//	return;
    int width = 1238;// media_info->video->width;
    int height = 480;// media_info->video->height;
#endif
	program = new GLShaderProgram();
	vao = new GLArray();
	vao->create();
#if 0   //vertex and texture
    vao->bind();
    bufVer = new GLBuffer(GLBuffer::VertexBuffer);
    bufVer->create();
    bufVer->bind();
    bufVer->allocate(vertexandtexture, sizeof(vertexandtexture));
    glVertexAttribPointer(A_VER, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid *)0);
    glEnableVertexAttribArray(A_VER);
    glVertexAttribPointer(T_VER, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid *)(2 * sizeof (float)));
    glEnableVertexAttribArray(T_VER);
    vao->unbind();
#else
	vao->bind();
	bufVer = new GLBuffer(GLBuffer::VertexBuffer);
	bufVer->create();
	bufVer->bind();
	bufVer->allocate(ver, sizeof(ver));
    glVertexAttribPointer(A_VER, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid *)0);
	glEnableVertexAttribArray(A_VER);
    vao->unbind();

    vao->bind();
    bufTex = new GLBuffer(GLBuffer::VertexBuffer);
    bufTex->create();
    bufTex->bind();
    bufTex->allocate(tex, sizeof(tex));
    glVertexAttribPointer(T_VER, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid *)0);
	glEnableVertexAttribArray(T_VER);
	vao->unbind();
#endif

	program->addShaderFromSourceCode(GLShaderProgram::Vertex, vString);
	program->addShaderFromSourceCode(GLShaderProgram::Fragment, tString);
	program->bindAttributeLocation("vertexIn", A_VER);
	program->bindAttributeLocation("textureIn", T_VER);
	program->link();
	program->bind();

	//从shader中获取材质
	unis[0] = program->uniformLocation("tex_y");
	unis[1] = program->uniformLocation("tex_u");
	unis[2] = program->uniformLocation("tex_v");

	//创建材质
	glGenTextures(3, texs);

	//y
	glBindTexture(GL_TEXTURE_2D, texs[0]);
	//放大过滤，线性插值
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//创建材质显卡空间
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

	//u
	glBindTexture(GL_TEXTURE_2D, texs[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

	//v
	glBindTexture(GL_TEXTURE_2D, texs[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

	//分配材质内存空间
	datas[0] = new unsigned char[width * height];       //y
	datas[1] = new unsigned char[width * height / 4];   //u
	datas[2] = new unsigned char[width * height / 4];   //v

#ifdef TEST_YUV
	fp = fopen("E://out400x240.yuv", "rb");
	if (!fp) {
		printf("Read file failed!\n");
	}
#endif
#endif
}

void OpenglVideo::renderVideo(const VideoFrame &frame)
{
#ifdef RENDER_TEST
	if (!program || !program->isValid())
		return;
	program->bind();
	vao->bind();

#ifdef TEST_YUV
	if (!fp)
		return;
	if (feof(fp)) {
		fseek(fp, 0, SEEK_SET);
	}
	int width = 400, height = 240;
	fread(datas[0], 1, width * height, fp);
	fread(datas[1], 1, width * height / 4, fp);
	fread(datas[2], 1, width * height / 4, fp);
#else
	int type = frame.pixelFormat();
	int width = frame.width(), height = frame.height();
    if (width == frame.bytesPerLine(0))
    {
        memcpy(datas[0], frame.constBits(0), width * height);
        memcpy(datas[1], frame.constBits(1), width * height / 4);
        memcpy(datas[2], frame.constBits(2), width * height / 4);
    }
    else
    {
        for(int i = 0; i < height; i++) //Y
            memcpy(datas[0] + width*i, frame.constBits(0) + frame.bytesPerLine(0)*i, width);
        for (int i = 0; i < height/2; i++) //U
            memcpy(datas[1] + width/2*i, frame.constBits(1) + frame.bytesPerLine(1)*i, width / 2); //if not use "width / 2", the program will crash when exit. 2020-08-03
        for (int i = 0; i < height/2; i++) //V
            memcpy(datas[2] + width/2*i, frame.constBits(2) + frame.bytesPerLine(2)*i, width / 2);

    }
#endif

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texs[0]);  //0层绑定到y材质
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//copy the data to texture
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, datas[0]);
	glUniform1i(unis[0], 0);

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, texs[1]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[1]);
	glUniform1i(unis[1], 1);

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, texs[2]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, datas[2]);
	glUniform1i(unis[2], 2);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	vao->unbind();
	program->unBind();
#endif
}

void OpenglVideo::initialize()
{
	DPTR_D(OpenglVideo);
    //d->pixmap_render.initialize();
	fill(d->background);
	double b = 0, c = 0, h = 0, s = 0;
	if (d->material) {
		b = d->material->brightness();
		c = d->material->contrast();
		h = d->material->hue();
		s = d->material->saturation();
		delete d->material;
		d->material = nullptr;
	}
	d->resetGL();
	d->material = new VideoMaterial();
	d->material->setBrightness(b);
	d->material->setContrast(c);
	d->material->setHue(h);
	d->material->setSaturation(s);
	updateViewport();
	if (d->manager)
		return;
	d->manager = new ShaderManager();
	static bool sInfo = true;
	if (sInfo) {
		sInfo = false;
		bool v;
        AVDebug("GL_VERSION: %s\n", glGetString(GL_VERSION));
        AVDebug("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
        AVDebug("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
        AVDebug("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
		const int glsl_ver = OpenglAide::GLSLVersion();
		AVDebug("GLSL version: %d", glsl_ver);
        v = OpenglAide::isPBOSupported();
        AVDebug("Has PBO: %d\n", v);
		v = OpenglAide::has16BitTexture();
        AVDebug("Has 16bit texture: %d\n", v);
		v = OpenglAide::hasRG();
        AVDebug("Has RG texture: %d\n", v);
	}
}

void OpenglVideo::setCurrentFrame(const VideoFrame & frame)
{
	DPTR_D(OpenglVideo);
	d->material->setCurrentFrame(frame);
	d->has_a = frame.format().hasAlpha();
}

void OpenglVideo::render(const RectF &target, const RectF& roi, const Matrix4x4& transform)
{
	DPTR_D(OpenglVideo);

    const int mt = d->material->type();
	if (d->material_type != mt) {
        AVDebug("material changed: %s => %s\n", VideoMaterial::typeName(d->material_type).c_str()
            , VideoMaterial::typeName(mt).c_str());
		d->material_type = mt;
	}
	if (!d->material->bind()) // bind first because texture parameters(target) mapped from native buffer is unknown before it
		return;
	VideoShader *shader = d->user_shader;
	if (!shader)
		shader = d->manager->prepareMaterial(d->material, mt); //TODO: print shader type name if changed-> prepareMaterial(,sample_code, pp_code)
    fill(d->background);
    glViewport(d->rect.x(), d->rect.y(), d->rect.width(), d->rect.height()); // viewport was used in gpu filters is wrong, qt quick fbo item's is right(so must ensure setProjectionMatrixToRect was called correctly)
	shader->update(d->material);
	shader->program()->setUniformValue(shader->matrixLocation(), transform*d->matrix);
	// uniform end-> attribute begin
	d->updateGeometry(shader, target, roi);
	// normalize?
	const bool blending = d->has_a;
	if (blending) {
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //
	}
	//if (d->mesh_type == OpenGLVideo::SphereMesh)
		//DYGL(glEnable(GL_CULL_FACE)); // required for sphere! FIXME: broken in qml and qgvf
	d->gr->render();
	if (blending)
        glDisable(GL_BLEND);
    d->material->unbind();
    shader->program()->unBind();

    // subtitle test
    //d->sub_render.drawText("Have a nice day\n hahahahaha");
    //d->pixmap_render.renderPixmap();
}

void OpenglVideo::updateViewport()
{
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //gluPerspective(90, 800.0f / 480.0f, 1, 1000);
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //gluLookAt(0.0, 0.0, 800.0f / 2.0f, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    //glRasterPos2i((long)(800 / 2 + -300), (long)(480 / 2 + 170));
    //glTranslatef(800 / 2, 480 / 2, 0.0);
}

void OpenglVideo::fill(const Color &c)
{
    DPTR_D(OpenglVideo);
    d->background = c;
    glClearColor(c.r, c.g, c.b, c.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenglVideo::setViewPort(RectF roi)
{
	DPTR_D(OpenglVideo);
	d->rect = roi;
	if (d->norm_viewport) {
		d->matrix.setToIdentity();
		if (d->mesh_type == SphereMesh)
			d->matrix.perspective(45, 1, 0.1, 100); // for sphere
	}
	else {
		d->matrix.setToIdentity();
		d->matrix.ortho(roi);
		d->update_geo = true; // even true for target_rect != d.rect
	}
	glViewport(d->rect.x(), d->rect.y(), d->rect.width(), d->rect.height());
}

NAMESPACE_END
