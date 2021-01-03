#include "VideoMaterial.h"
#include "OpenglAide.h"
#include "glpackage.h"
#include "innermath.h"
#include "ColorTransform.h"
#include "glad/glad.h"
#include "Size.h"
#include "AVLog.h"

#include <sstream>
#include <vector>
#include <math.h>

#define YUVA_DONE 0

NAMESPACE_BEGIN

class VideoMaterialPrivate
{
public:
	VideoMaterialPrivate() :
        width(0),height(0),
        bpc(0),
        video_format(VideoFormat::Format_Invalid),
		plane1_linesize(0),
        update_texture(true),
        init_textures_required(true),
		effective_tex_width_ratio(1.0),
        target(GL_TEXTURE_2D),
        dirty(true),
        try_pbo(true)
	{
		textures.reserve(4);
		texture_size.reserve(4);
		internal_format.reserve(4);
		data_format.reserve(4);
		data_type.reserve(4);
	}

	bool initTexture(GLuint tex, GLint internal_format, GLenum format,
		GLenum dataType, int width, int height);
    bool initPBO(unsigned int plane, int size);
	void setupQuality();
    bool updateTextureParameters(const VideoFormat& fmt);
	bool ensureResources();
	bool ensureTextures();
    void uploadPlane(unsigned int p, bool updateTexture = true);

	int width, height;
	int bpc;
	VideoFrame frame;
	VideoFormat video_format;
	Size plane0Size;
	// width is in bytes. different alignments may result in different plane 1 linesize even if plane 0 are the same
	int plane1_linesize;

	bool update_texture; // reduce upload/map times. true: new frame not bound. false: current frame is bound
	bool init_textures_required;
	std::vector<GLuint> textures;
	std::map<GLuint, bool> owns_texture;
	std::vector<Size> texture_size;
	std::vector<int> effective_tex_width; //without additional width for alignment
	double effective_tex_width_ratio;
	GLenum target;
	/**
	 * What format does OpenGL use to store and use this texture data?
	 * for example YUV is GL_LUMINANCE, the others are GL_APHPA, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
	 */
	std::vector<GLint> internal_format;
	std::vector<GLenum> data_format;
	std::vector<GLenum> data_type;
	Vector2D vec_to8; //TODO: vec3 to support both RG and LA (.rga, vec_to8)
	Matrix4x4 channel_map;
	std::vector<Vector2D> v_texel_size;
	std::vector<Vector2D> v_texture_size;

	bool dirty;
	ColorTransform colorTransform;
    //PBO
    bool try_pbo;
    std::vector<GLBuffer> pbos;

};

bool VideoMaterialPrivate::initTexture(GLuint tex, GLint internal_format, GLenum format, 
	GLenum dataType, int width, int height)
{
	glBindTexture(target, tex);
    setupQuality();
	// This is necessary for non-power-of-two textures
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(target, 0, internal_format, width, height, 0/*border, ES not support*/, format, dataType, NULL);
	glBindTexture(target, 0);
	return true;
}

bool VideoMaterialPrivate::initPBO(unsigned int plane, int size)
{
    GLBuffer &pb = pbos[plane];
    if (!pb.isCreated()) {
        AVDebug("Creating PBO for plane %d, size: %d...\n", plane, size);
        pb.create();
    }
    if (!pb.bind()) {
        AVWarning("Failed to bind PBO for plane %d!!!!!!\n", plane);
        try_pbo = false;
        return false;
    }
    //pb.setUsagePattern(QOpenGLBuffer::DynamicCopy);
    AVDebug("Allocate PBO size %d\n", size);
    pb.allocate(size);
    pb.unbind();
    return true;
}

void VideoMaterialPrivate::setupQuality()
{
	// Zoom filter, linear interpolation
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

bool VideoMaterialPrivate::updateTextureParameters(const VideoFormat& fmt)
{
	if (!fmt.isValid())
		return false;
    const unsigned int nb_planes = fmt.planeCount();
	internal_format.resize(nb_planes);
	data_format.resize(nb_planes);
	data_type.resize(nb_planes);
	if (!OpenglAide::videoFormatToGL(fmt, (GLint*)internal_format.data(), (GLenum*)data_format.data(),
		(GLenum*)data_type.data(), &channel_map)) {
        AVWarning("No OpenGL support for %s.\n", fmt.name().c_str());
		return false;
	}
    for (unsigned int i = 0; i < nb_planes; ++i) {
        const int bpp_gl = OpenglAide::bytesOfTexel(data_format[i], data_type[i]);
        const int pad = std::ceil((double)(texture_size[i].width - effective_tex_width[i])/(double)bpp_gl);
        texture_size[i].width = (std::ceil((double)texture_size[i].width / (double)bpp_gl));
        effective_tex_width[i] /= bpp_gl; //fmt.bytesPerPixel(i);
        v_texture_size[i] = Vector2D(texture_size[i].width, texture_size[i].height);
        //effective_tex_width_ratio =
        AVDebug("texture width: %d - %d = pad: %d. bpp(gl): %d", texture_size[i].width, effective_tex_width[i], pad, bpp_gl);
        if (target == GL_TEXTURE_RECTANGLE)
            v_texel_size[i] = Vector2D(1.0f, 1.0f);
        else
            v_texel_size[i] = Vector2D(1.0f/(float)texture_size[i].width, 1.0f/(float)texture_size[i].height);
    }
    /*
     * there are 2 fragment shaders: rgb and yuv.
     * only 1 texture for packed rgb. planar rgb likes yuv
     * To support both planar and packed yuv, and mixed yuv(NV12), we give a texture sample
     * for each channel. For packed, each (channel) texture sample is the same. For planar,
     * packed channels has the same texture sample.
     * But the number of actural textures we upload is plane count.
     * Which means the number of texture id equals to plane count
     */
    // always delete old textures otherwise old textures are not initialized with correct parameters
    if (textures.size() > nb_planes) { //TODO: why check this?
        const int nb_delete = textures.size() - nb_planes;
        AVDebug("try to delete %d textures", nb_delete);
        if (!textures.empty()) {
            for (int i = 0; i < nb_delete; ++i) {
                GLuint &t = textures[nb_planes+i];
                AVDebug("try to delete texture[%d]: %u. can delete: %d", nb_planes+i, t, owns_texture[t]);
                if (owns_texture[t])
                    glDeleteTextures(1, &t);
            }
            //DYGL(glDeleteTextures(nb_delete, textures.data() + nb_planes));
        }
        owns_texture.clear();
    }
    textures.resize(nb_planes);
    init_textures_required = true;
	return true;
}

bool VideoMaterialPrivate::ensureResources()
{
	if (!update_texture) //video frame is already uploaded and displayed
        return true;
    const VideoFormat &fmt = video_format;
    if (!fmt.isValid())
        return false;
    // update textures if format, texture target, valid texture width(normalized), plane 0 size or plane 1 line size changed
    bool update_textures = init_textures_required;
    const int nb_planes = fmt.planeCount();
    // effective size may change even if plane size not changed
    bool effective_tex_width_ratio_changed = true;
    for (int i = 0; i < nb_planes; ++i) {
        if ((double)frame.effectiveBytesPerLine(i)/(double)frame.bytesPerLine(i) == effective_tex_width_ratio) {
            effective_tex_width_ratio_changed = false;
            break;
        }
    }
    const int linsize0 = frame.bytesPerLine(0);
    if (update_textures
            || effective_tex_width_ratio_changed
            || linsize0 != plane0Size.width || frame.height() != plane0Size.height
            || (plane1_linesize > 0 && frame.bytesPerLine(1) != plane1_linesize)) { // no need to check height if plane 0 sizes are equal?
        update_textures = true;
        dirty = true;
        v_texel_size.resize(nb_planes);
        v_texture_size.resize(nb_planes);
        texture_size.resize(nb_planes);
        effective_tex_width.resize(nb_planes);
        effective_tex_width_ratio = 1.0;
        for (int i = 0; i < nb_planes; ++i) {
            AVDebug("plane linesize %d: padded = %d, effective = %d. theoretical plane size: %dx%d", i, frame.bytesPerLine(i), frame.effectiveBytesPerLine(i), frame.planeWidth(i), frame.planeHeight(i));
            // we have to consider size of opengl format. set bytesPerLine here and change to width later
            texture_size[i] = Size(frame.bytesPerLine(i), frame.planeHeight(i));
            effective_tex_width[i] = frame.effectiveBytesPerLine(i); //store bytes here, modify as width later
            // usually they are the same. If not, the difference is small. min value can avoid rendering the invalid data.
            effective_tex_width_ratio = std::min(effective_tex_width_ratio, (double)frame.effectiveBytesPerLine(i)/(double)frame.bytesPerLine(i));
        }
        plane1_linesize = 0;
        if (nb_planes > 1) {
            // height? how about odd?
            plane1_linesize = frame.bytesPerLine(1);
        }
        /*
          let wr[i] = valid_bpl[i]/bpl[i], frame from avfilter maybe wr[1] < wr[0]
          e.g. original frame plane 0: 720/768; plane 1,2: 360/384,
          filtered frame plane 0: 720/736, ... (16 aligned?)
         */
        AVDebug("effective_tex_width_ratio=%f.\n", effective_tex_width_ratio);
        plane0Size.width = linsize0;
        plane0Size.height = frame.height();
    }
    if (update_textures) {
        updateTextureParameters(fmt);
		//TODO use pbo 2020-08-02
        // check pbo support
        try_pbo = try_pbo && OpenglAide::isPBOSupported();
        // check PBO support with bind() is fine, no need to check extensions
        if (try_pbo) {
            pbos.resize(nb_planes);
            for (int i = 0; i < nb_planes; ++i) {
                AVDebug("Init PBO for plane %d\n", i);
                pbos[i] = GLBuffer(GLBuffer::PixelUnpackBuffer); //QOpenGLBuffer is shared, must initialize 1 by 1 but not use fill
                if (!initPBO(i, frame.bytesPerLine(i)*frame.planeHeight(i))) {
                    AVWarning("Failed to init PBO for plane %d\n", i);
                    break;
                }
            }
        }
    }
	return true;
}

bool VideoMaterialPrivate::ensureTextures()
{
	if (!init_textures_required)
		return true;
	// create in bindPlane loop will cause wrong texture binding
	const int nb_planes = video_format.planeCount();
	for (int p = 0; p < nb_planes; ++p) {
		GLuint &tex = textures[p];
		if (tex) { // can be 0 if resized to a larger size
			AVDebug("try to delete texture for plane %d (id=%u). can delete: %d", p, tex, owns_texture[tex]);
			if (owns_texture[tex])
				glDeleteTextures(1, &tex);
			owns_texture.erase(tex);
			tex = 0;
		}
		if (!tex) {
			AVDebug("creating texture for plane %d.\n", p);
			//GLuint* handle = (GLuint*)frame.createInteropHandle(&tex, GLTextureSurface, p); // take the ownership
			//if (handle) {
			//	tex = *handle;
			//	owns_texture[tex] = true;
			//}
			//else {
				glGenTextures(1, &tex);
				owns_texture[tex] = true;
				initTexture(tex, internal_format[p], data_format[p], data_type[p], texture_size[p].width, texture_size[p].height);
			//}
			AVDebug("texture for plane %d is created (id=%u)\n", p, tex);
		}
	}
	init_textures_required = false;
	return true;
}

void VideoMaterialPrivate::uploadPlane(unsigned int p, bool updateTexture)
{
    GLuint &tex = textures[p];
    glActiveTexture(GL_TEXTURE0 + p); //0 must active?
    if (!updateTexture) {
        glBindTexture(target, tex);
        return;
    }
    //TODO hardware decode 2020-08-02
    //if (!frame.constBits(0)) {
    //	// try_pbo ? pbo_id : 0. 0= > interop.createHandle
    //	GLuint tex0 = tex;
    //	if (frame.map(GLTextureSurface, &tex, p)) {
    //		if (tex0 != tex) {
    //			if (owns_texture[tex0])
    //				glDeleteTextures(1, &tex0);
    //			owns_texture.erase(tex0);
    //			owns_texture[tex] = false;
    //		}
    //		glBindTexture(target, tex); // glActiveTexture was called, but maybe bind to 0 in map
    //		return;
    //	}
    //	tWarning("map hw surface error");
    //	return;
    //}
    // FIXME: why happens on win?
    if (frame.bytesPerLine(p) <= 0)
        return;
    if (try_pbo) {
        //qDebug("bind PBO %d", p);
        GLBuffer &pb = pbos[p];
        pb.bind();
        // glMapBuffer() causes sync issue.
        // Call glBufferData() with NULL pointer before glMapBuffer(), the previous data in PBO will be discarded and
        // glMapBuffer() returns a new allocated pointer or an unused block immediately even if GPU is still working with the previous data.
        // https://www.opengl.org/wiki/Buffer_Object_Streaming#Buffer_re-specification
        pb.allocate(pb.size());
        GLubyte* ptr = (GLubyte*)pb.map(GLBuffer::WriteOnly);
        if (ptr) {
            memcpy(ptr, frame.constBits(p), pb.size());
            pb.unmap();
        }
    }
    //qDebug("bpl[%d]=%d width=%d", p, frame.bytesPerLine(p), frame.planeWidth(p));
    glBindTexture(target, tex);
    glTexSubImage2D(target, 0, 0, 0, texture_size[p].width, texture_size[p].height,
                    data_format[p], data_type[p], try_pbo ? nullptr : frame.constBits(p));
    if (try_pbo) {
        pbos[p].unbind();
    }
}

VideoMaterial::VideoMaterial():
	d_ptr(new VideoMaterialPrivate)
{

}

void VideoMaterial::setCurrentFrame(const VideoFrame & frame)
{
	DPTR_D(VideoMaterial);
	d->update_texture = true;
	// TODO: move to another function before rendering?
	d->width = frame.width();
	d->height = frame.height();
	GLenum new_target = GL_TEXTURE_2D; // not d->target. because metadata "target" is not always set
	//QByteArray t = frame.metaData(QStringLiteral("target")).toByteArray().toLower();
	//if (t == QByteArrayLiteral("rect"))
	//	new_target = GL_TEXTURE_RECTANGLE;
	//if (new_target != d->target) {
	//	qDebug("texture target: %#x=>%#x", d->target, new_target);
	//	// FIXME: not thread safe (in qml)
	//	d->target = new_target;
	//	d->init_textures_required = true;
	//}
	// TODO: check hw interop change. if change from an interop owns texture to not owns texture, VideoShader must recreate textures because old textures are deleted by previous interop
	const VideoFormat fmt(frame.format());
	const int bpc_old = d->bpc;
	d->bpc = fmt.bitsPerComponent();
	if (d->bpc > 8 && (d->bpc != bpc_old || d->video_format.isBigEndian() != fmt.isBigEndian())) {
		//FIXME: Assume first plane has 1 channel. So not work with NV21
		const int range = (1 << d->bpc) - 1;
		// FFmpeg supports 9, 10, 12, 14, 16 bits
		// 10p in little endian: yyyyyyyy yy000000 => (L, L, L, A)  //(yyyyyyyy, 000000yy)?
		if (OpenglAide::depth16BitTexture() < 16 || !OpenglAide::has16BitTexture() || fmt.isBigEndian()) {
			if (fmt.isBigEndian())
				d->vec_to8 = Vector2D(256.0, 1.0)*255.0 / (float)range;
			else
				d->vec_to8 = Vector2D(1.0, 256.0)*255.0 / (float)range;
			d->colorTransform.setChannelDepthScale(1.0);
		}
		else {
			/// 16bit (R16 e.g.) texture does not support >8bit be channels
			/// 10p be: R2 R1(Host) = R1*2^8+R2 = 000000rr rrrrrrrr ->(GL) R=R2*2^8+R1
			/// 10p le: R1 R2(Host) = rrrrrrrr rr000000
			//d->vec_to8 = QVector2D(1.0, 0.0)*65535.0/(float)range;
			d->colorTransform.setChannelDepthScale(65535.0 / (double)range, YUVA_DONE && fmt.hasAlpha());
		}
	}
	else {
		if (d->bpc <= 8)
			d->colorTransform.setChannelDepthScale(1.0);
	}
	// http://forum.doom9.org/archive/index.php/t-160211.html
	ColorSpace cs = frame.colorSpace();// ColorSpace_RGB;
	if (cs == ColorSpace_Unknown) {
		if (fmt.isRGB()) {
			if (fmt.isPlanar())
				cs = ColorSpace_GBR;
			else
				cs = ColorSpace_RGB;
		}
		else if (fmt.isXYZ()) {
			cs = ColorSpace_XYZ;
		}
		else {
			if (frame.width() >= 1280 || frame.height() > 576) //values from mpv
				cs = ColorSpace_BT709;
			else
				cs = ColorSpace_BT601;
		}
	}
	d->colorTransform.setInputColorSpace(cs);
	d->colorTransform.setInputColorRange(frame.colorRange());
	// TODO: use graphics driver's color range option if possible
	static const ColorRange kRgbDispRange = getenv("QTAV_DISPLAY_RGB_RANGE") == "limited" ? ColorRange_Limited : ColorRange_Full;
	d->colorTransform.setOutputColorRange(kRgbDispRange);
	d->frame = frame;
	if (fmt != d->video_format) {
		AVDebug(fmt.name().c_str());
		//tDebug("pixel format changed: %s => %s %d", qPrintable(d->video_format.name()), qPrintable(fmt.name()), fmt.pixelFormat());
		d->video_format = fmt;
		d->init_textures_required = true;
	}
}

VideoFormat VideoMaterial::currentFormat() const
{
	DPTR_D(const VideoMaterial);
	return d->video_format;
}

int VideoMaterial::textureTarget() const
{
	DPTR_D(const VideoMaterial);
	return d->target;
}
//
//VideoShader* VideoMaterial::createShader() const
//{
//	VideoShader *shader = new VideoShader();
//	// initialize shader
//	shader->setVideoFormat(currentFormat());
//	shader->setTextureTarget(textureTarget());
//	shader->setMaterialType(type());
//	//resize texture locations to avoid access format later
//	return shader;
//}

int VideoMaterial::type() const
{
	DPTR_D(const VideoMaterial);
	const VideoFormat &fmt = d->video_format;
	const bool tex_2d = d->target == GL_TEXTURE_2D;
	// 2d,alpha,planar,8bit
	const int rg_biplane = fmt.planeCount() == 2 && !OpenglAide::useDeprecatedFormats() && OpenglAide::hasRG();
	const int channel16_to8 = d->bpc > 8 && (OpenglAide::depth16BitTexture() < 16 || !OpenglAide::has16BitTexture() || fmt.isBigEndian());
	return (fmt.isXYZ() << 5) | (rg_biplane << 4) | (tex_2d << 3) | (fmt.hasAlpha() << 2) | (fmt.isPlanar() << 1) | (channel16_to8);
}

std::string VideoMaterial::typeName(int value)
{
	std::stringstream s;
	s << "gl material 16to8bit: " << (!!(value & 1));
	s << ", planar: " << (!!(value & (1 << 1)));
	s << ", has alpha: " << (!!(value & (1 << 2)));
	s << ", 2d texture: " << (!!(value & (1 << 3)));
	s << ", 2nd plane rg: " << (!!(value & (1 << 4)));
	s << ", xyz: " << (!!(value & (1 << 5)));
	return s.str();
}

bool VideoMaterial::bind()
{
	DPTR_D(VideoMaterial);
	if (!d->ensureResources())
		return false;
    unsigned int nb_planes = d->textures.size(); //number of texture id
    if (nb_planes <= 0 || nb_planes > 4)
		return false;
	d->ensureTextures();
    //write data to GPU
    for (unsigned int i = 0; i < nb_planes; ++i) {
        const unsigned int p = i % nb_planes;
		d->uploadPlane(p, d->update_texture);
	}
	return true;
}

void VideoMaterial::unbind()
{
	DPTR_D(VideoMaterial);
	const int nb_planes = d->textures.size(); //number of texture id
	for (int i = 0; i < nb_planes; ++i) {
		// unbind planes in the same order as bind. GPU frame's unmap() can be async works, assume the work finished earlier if it started in map() earlier, thus unbind order matter
		const int p = (i + 1) % nb_planes; //0 must active at last?
		d->frame.unmap(&d->textures[p]);
	}
	if (d->update_texture) {
		d->update_texture = false;
		d->frame = VideoFrame(); //FIXME: why need this? we must unmap correctly before frame is reset.
	}
	setDirty(false);
}

void VideoMaterial::setDirty(bool value)
{
	d_func()->dirty = value;
}

bool VideoMaterial::isDirty() const
{
	return d_func()->dirty;
}

const Matrix4x4 & VideoMaterial::colorMatrix() const
{
	return d_func()->colorTransform.matrixRef();
}

const Matrix4x4 & VideoMaterial::channelMap() const
{
	return d_func()->channel_map;
}

Vector2D VideoMaterial::vectorTo8bit() const
{
	return d_func()->vec_to8;
}

double VideoMaterial::validTextureWidth() const
{
	return d_func()->effective_tex_width_ratio;
}

Size VideoMaterial::frameSize() const
{
	return Size(d_func()->width, d_func()->height);
}

SizeF VideoMaterial::texelSize(int plane) const
{
	DPTR_D(const VideoMaterial);
	return SizeF(1.0 / (double)d->texture_size[plane].width, 1.0 / (double)d->texture_size[plane].height);
}

std::vector<Vector2D> VideoMaterial::texelSize() const
{
	return d_func()->v_texel_size;
}

Size VideoMaterial::textureSize(int plane) const
{
	return d_func()->texture_size[plane];
}
std::vector<Vector2D> VideoMaterial::textureSize() const
{
	return d_func()->v_texture_size;
}

double VideoMaterial::brightness() const
{
	return d_func()->colorTransform.brightness();
}

void VideoMaterial::setBrightness(double value)
{
	d_func()->colorTransform.setBrightness(value);
	d_func()->dirty = true;
}

double VideoMaterial::contrast() const
{
	return d_func()->colorTransform.contrast();
}

void VideoMaterial::setContrast(double value)
{
	d_func()->colorTransform.setContrast(value);
	d_func()->dirty = true;
}

double VideoMaterial::hue() const
{
	return d_func()->colorTransform.hue();
}

void VideoMaterial::setHue(double value)
{
	d_func()->colorTransform.setHue(value);
	d_func()->dirty = true;
}

double VideoMaterial::saturation() const
{
	return d_func()->colorTransform.saturation();
}

void VideoMaterial::setSaturation(double value)
{
	d_func()->colorTransform.setSaturation(value);
	d_func()->dirty = true;
}

RectF VideoMaterial::mapToTexture(int plane, const RectF & roi, int normalize) const
{
	DPTR_D(const VideoMaterial);
	if (d->texture_size.empty()) { //It should not happen if it's called in QtAV
		AVWarning("VideoMaterial::mapToTexture->textures not ready\n");
		return RectF();
	}
	const double tex0W = d->texture_size[0].width;
	const double s = tex0W / double(d->width); // only apply to unnormalized input roi
	const double pw = d->video_format.normalizedWidth(plane);
	const double ph = d->video_format.normalizedHeight(plane);
	if (normalize < 0)
		normalize = d->target != GL_TEXTURE_RECTANGLE;
	if (!roi.isValid()) {
		if (normalize)
			return RectF(0, 0, d->effective_tex_width_ratio, 1); //NOTE: not (0, 0, 1, 1)
		return RectF(0, 0, tex0W*pw, d->height * ph);
	}
	float x = roi.x();
	float w = roi.width(); //TODO: texturewidth
	float y = roi.y();
	float h = (float)roi.height();
	if (normalize) {
		if (std::abs(x) > 1) {
			x /= tex0W;
			x *= s;
		}
		if (std::abs(y) > 1)
			y /= (float)d->height;
		if (std::abs(w) > 1) {
			w /= tex0W;
			w *= s;
		}
		if (std::abs(h) > 1)
			h /= (float)d->height;
	}
	else { //FIXME: what about ==1?
		if (std::abs(x) <= 1)
			x *= tex0W;
		else
			x *= s;
		if (std::abs(y) <= 1)
			y *= (float)d->height;
		if (std::abs(w) <= 1)
			w *= tex0W;
		else
			w *= s;
		if (std::abs(h) <= 1)
			h *= (float)d->height;
	}
	// multiply later because we compare with 1 before it
	x *= d->effective_tex_width_ratio;
	w *= d->effective_tex_width_ratio;
	return RectF(x*pw, y*ph, w*pw, h*ph);
}

NAMESPACE_END
