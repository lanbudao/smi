#ifndef OPENGL_VIDEO_H
#define OPENGL_VIDEO_H

#include "glad/glad.h"
#include "sdk/DPTR.h"
#include <shared_mutex>
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif
#include "renderer/glpackage.h"
#include "sdk/mediainfo.h"
#include "VideoFrame.h"
#include "Matrix4x4.h"
#include "RectF.h"

#define A_VER 0
#define T_VER 1
#define BUFFER_OFFSET(offset) ((void*)(offset))
#define GET_STR(x) #x

//#define RENDER_TEST

NAMESPACE_BEGIN

static const char* vString = GET_STR(
	attribute vec4 vertexIn;
	attribute vec2 textureIn;
	varying vec2 textureOut;
	void main(void) {
		gl_Position = vertexIn;
		textureOut = textureIn;
	}
);

static const char* tString = GET_STR(
	varying vec2 textureOut;
	uniform sampler2D tex_y;
	uniform sampler2D tex_u;
	uniform sampler2D tex_v;

	void main(void) {
		vec3 yuv;
		vec3 rgb;
		yuv.x = texture2D(tex_y, textureOut).r;
		yuv.y = texture2D(tex_u, textureOut).r - 0.5;
		yuv.z = texture2D(tex_v, textureOut).r - 0.5;
		rgb = mat3(1.0, 1.0, 1.0,
			0, -0.39465, 2.03211,
			1.13983, -0.58060, 0.0) * yuv;
		gl_FragColor = /*vec4(1.0, 1.0, 0.0, 1.0)*/vec4(rgb, 1.0);
	}
);

class OpenglVideoPrivate;
class OpenglVideo
{
	DPTR_DECLARE_PRIVATE(OpenglVideo);
public:
	enum MeshType {
		RectMesh,
		SphereMesh
	};
	OpenglVideo();
	~OpenglVideo();

	void setMediaInfo(MediaInfo *info);
	void initGL();

	void renderVideo(const VideoFrame &frame);

	void initialize();
	void setCurrentFrame(const VideoFrame& frame);
	void render(const RectF &target, const RectF& roi, const Matrix4x4& transform);
	void updateViewport();
	/**
	 * @brief Set background color
	 */
    void fill(const Color &c);

	void setViewPort(RectF roi);

private:
	DPTR_DECLARE(OpenglVideo);

private:
#ifdef RENDER_TEST
	MediaInfo *media_info;
	GLShaderProgram* program;
	GLArray *vao;
	GLBuffer *bufVer, *bufTex;
	//shader中的yuv变量地址
	int unis[3] = { 0 };
	//opengl中的材质地址
	GLuint texs[3] = { 0 };
	//材质内存空间
	unsigned char *datas[3] = { nullptr };

	std::mutex mtx;

	SwsContext *img_convert_ctx = nullptr;
	AVFrame *frame_yuv = nullptr;

	FILE *fp = nullptr;
#endif
};
NAMESPACE_END
#endif //OPENGL_VIDEO_H
