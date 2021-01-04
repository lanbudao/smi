#include "VideoShader.h"
#include "glad/glad.h"
#include "AVLog.h"
#include "OpenglAide.h"
#include "glpackage.h"
#include "VideoMaterial.h"

#include <sstream>
#include <vector>
#include <fstream>
#include <regex>
#include <exception>

#define YUVA_DONE 0

NAMESPACE_BEGIN

static std::string commonShaderHeader(GLShaderProgram::ShaderType type)
{
    // TODO: check useDeprecatedFormats() or useDeprecated()?
    std::string h;
    if (OpenglAide::isOpenGLES()) {
        h += "precision mediump int;\n"
            "precision mediump float;\n"
            ;
    }
    else {
        h += "#define highp\n"
            "#define mediump\n"
            "#define lowp\n"
            ;
    }
    if (type == GLShaderProgram::Fragment) {
        // >=1.30: texture(sampler2DRect,...). 'texture' is defined in header
        // we can't check GLSLVersion() here because it the actually version used can be defined by "#version"
        h += "#if __VERSION__ < 130\n"
            "#define texture texture2D\n"
            "#else\n"
            "#define texture2D texture\n"
            "#endif\n"
            ;
    }
    return h;
}

static std::string compatibleShaderHeader(GLShaderProgram::ShaderType type)
{
    if (OpenglAide::isOpenGLES())
        return commonShaderHeader(type);
    std::string h;
    int ver = OpenglAide::GLSLVersion();
    std::stringstream s;
    s << ver;
    h.append("#version ").append(s.str()).append("\n");
    h += commonShaderHeader(type);
    if (OpenglAide::GLSLVersion() >= 130) { // gl(es) 3
        if (type == GLShaderProgram::Vertex) {
            h += "#define attribute in\n"
                "#define varying out\n"
                ;
        }
        else if (type == GLShaderProgram::Fragment) {
            h += "#define varying in\n"
                "#define gl_FragColor out_color\n"  //can not starts with 'gl_'
                "out vec4 gl_FragColor;\n"
                ;
        }
    }
    return h;
}

class VideoShaderPrivate
{
public:
	VideoShaderPrivate() :
		program(nullptr),
		rebuild_program(false),
		update_builtin_uniforms(true),
		u_Matrix(-1),
		u_colorMatrix(-1),
		u_to8(-1),
		u_opacity(-1),
		u_c(-1),
		material_type(0),
		texture_target(GL_TEXTURE_2D)
	{

	}
	GLShaderProgram* program;
	bool rebuild_program;
	bool update_builtin_uniforms; //builtin uniforms are static, set the values once is enough if no change
	std::vector<int> u_Texture;
	int u_Matrix;
	int u_colorMatrix;
	int u_to8;
	int u_opacity;
	int u_c;
	int u_texelSize;
	int u_textureSize;

	int material_type;
	GLenum texture_target;
	VideoFormat video_format;

	mutable std::string planar_frag, packed_frag;
	mutable std::string vert;
};

VideoShader::VideoShader():
	d_ptr(new VideoShaderPrivate)
{

}

VideoShader::~VideoShader()
{

}

std::string VideoShader::shaderSourceFromFile(const std::string &fileName) const
{
	std::fstream file;
	file.open(fileName.c_str(), std::fstream::in);
	if (!file.is_open()) {
        AVError("Can not load shader: %s!\n", fileName.c_str());
		return "";
	}
	std::ostringstream os;
	os << file.rdbuf();
	return os.str();
}

char const * const * VideoShader::attributeNames() const
{
	DPTR_D(const VideoShader);

	static const char *names[] = {
		"a_Position",
		"a_TexCoords0",
		0
	};
	if (textureTarget() == GL_TEXTURE_2D)
		return names;
	static const char *names_multicoord[] = {
		"a_Position",
		"a_TexCoords0",
		"a_TexCoords1",
		"a_TexCoords2",
		0
	};
#if YUVA_DONE
	static const char *names_multicoord_4[] = {
		"a_Position",
		"a_TexCoords0",
		"a_TexCoords1",
		"a_TexCoords2",
		"a_TexCoords3",
		0
	};
	if (d_func().video_format.planeCount() == 4)
		return names_multicoord_4;
#endif
	// TODO: names_multicoord_4planes
	return d->video_format.isPlanar() ? names_multicoord : names;
}

std::string VideoShader::vertexShader() const
{
	DPTR_D(const VideoShader);

    std::string s;
    bool multiCoords = false, hasAlpha = false;
    if (textureTarget() == GL_TEXTURE_RECTANGLE && d->video_format.isPlanar())
        multiCoords = true;
    hasAlpha = d->video_format.hasAlpha();

    d->vert.append("attribute vec4 a_Position;\n"
                   "attribute vec2 a_TexCoords0;\n"
                   "uniform mat4 u_Matrix;\n"
                   "varying vec2 v_TexCoords0;\n");
    if (multiCoords)
        d->vert.append("attribute vec2 a_TexCoords1;\n"
                        "attribute vec2 a_TexCoords2;\n"
                        "varying vec2 v_TexCoords1;\n"
                        "varying vec2 v_TexCoords2;\n");
#if YUVA_DONE
        if (hasAlpha)
            d->vert.append("attribute vec2 a_TexCoords3;"
                    "varying vec2 v_TexCoords3;");
#endif
    d->vert.append("void main() {\n"
                    "gl_Position = u_Matrix * a_Position;\n"
                    "v_TexCoords0 = a_TexCoords0;\n");
    if (multiCoords) {
        d->vert.append("v_TexCoords1 = a_TexCoords1;\n"
                       "v_TexCoords2 = a_TexCoords2;\n");
        if (hasAlpha)
            d->vert.append("v_TexCoords3 = a_TexCoords3;\n");
    }
    d->vert.append("}\n");
    d->vert.insert(0, compatibleShaderHeader(GLShaderProgram::Vertex));
    AVDebug("vertexShader: %s.\n", d->vert.c_str());
    return d->vert;
}

std::string VideoShader::fragmentShader() const
{
	DPTR_D(const VideoShader);

    bool useRG = (OpenglAide::hasRG() && !OpenglAide::useDeprecatedFormats());
    bool channel16To8 = false;
    bool hasAlpha = d->video_format.hasAlpha();
    bool isXYZ = d->video_format.isXYZ();
    bool isTexRect = d->texture_target == GL_TEXTURE_RECTANGLE;

    if (d->video_format.bitsPerComponent() > 8) {
        //// has and use 16 bit texture (r16 for example): If channel depth is 16 bit, no range convertion required-> Otherwise, must convert to color.r*(2^16-1)/(2^bpc-1)
        if (OpenglAide::depth16BitTexture() < 16 || !OpenglAide::has16BitTexture() || d->video_format.isBigEndian())
            channel16To8 = true;
    }

    std::string& frag = d->video_format.isPlanar() ? d->planar_frag : d->packed_frag;
    if (d->video_format.isPlanar()) {
        frag.append("uniform sampler2D u_Texture0;\n"
                              "uniform sampler2D u_Texture1;\n"
                              "uniform sampler2D u_Texture2;\n");
        if (hasAlpha)
            frag.append("uniform sampler2D u_Texture3;\n");
        frag.append("varying vec2 v_TexCoords0;\n");
        if (isTexRect) {
            frag.append("varying vec2 v_TexCoords1;"
                                  "varying vec2 v_TexCoords2;");
            if (hasAlpha)
                frag.append("varying vec2 v_TexCoords3;\n");
        } else {
            frag.append("#define v_TexCoords1 v_TexCoords0\n"
                                  "#define v_TexCoords2 v_TexCoords0\n"
                                  "#define v_TexCoords3 v_TexCoords0\n");
        }
        frag.append("uniform float u_opacity;\n"
                              "uniform mat4 u_colorMatrix;\n");
        if (channel16To8)
            frag.append("uniform vec2 u_to8;\n");
        frag.append("vec4 sample2d(sampler2D tex, vec2 pos, int plane) {"
                                "return texture(tex, pos);}\n"
                              "void main(){\n"
                                "gl_FragColor = clamp(u_colorMatrix * vec4(");
        if (channel16To8) {
            if (useRG) {
                frag.append("dot(sample2d(u_Texture0, v_TexCoords0, 0).rg, u_to8),\n"
                                      "dot(sample2d(u_Texture1, v_TexCoords1, 1).rg, u_to8),\n"
                                      "dot(sample2d(u_Texture2, v_TexCoords2, 2).rg, u_to8),\n");
            } else {
                frag.append("dot(sample2d(u_Texture0, v_TexCoords0, 0).ra, u_to8),\n"
                                      "dot(sample2d(u_Texture1, v_TexCoords1, 1).ra, u_to8),\n"
                                      "dot(sample2d(u_Texture2, v_TexCoords2, 2).ra, u_to8),\n");
            }
        } else {
            if (useRG) {
                frag.append("sample2d(u_Texture0, v_TexCoords0, 0).r,\n"
                                      "sample2d(u_Texture1, v_TexCoords1, 1).r,\n"
                                      "sample2d(u_Texture2, v_TexCoords2, 2).");
                frag.append(isTexRect ? "g,\n" : "r,\n");
            } else {
                frag.append("sample2d(u_Texture0, v_TexCoords0, 0).r,\n"
                                      "sample2d(u_Texture1, v_TexCoords1, 1).g,\n"
                                      "sample2d(u_Texture2, v_TexCoords2, 2).a,\n");
            }
        }
        frag.append(" 1.0), 0.0, 1.0) * u_opacity;\n");
        if (hasAlpha) {
            frag.append(" float a = ");
            if (channel16To8) {
                frag.append("dot(sample2d(u_Texture3, v_TexCoords3, 3)");
                frag.append(useRG ? ".rg, u_to8);" : ".ra, u_to8);");
            } else {
                frag.append("sample2d(u_Texture3, v_TexCoords3, 3)")
                        .append(useRG ? ".r;" : ".a;");
            }
            frag.append("gl_FragColor.rgb = gl_FragColor.rgb * a;\n"
                                  "gl_FragColor.a = a;");
        }
        frag.append("}\n");
    } else {
        //packed.glsl
        frag.append("uniform sampler2D u_Texture0;\n"
                    "varying vec2 v_TexCoords0;\n"
                    "uniform mat4 u_colorMatrix;\n"
                    "uniform float u_opacity;\n"
                    "uniform mat4 u_c;\n"
                    "vec4 sample2d(sampler2D tex, vec2 pos, int plane)\n"
                    "{\n"
                    "    return texture(tex, pos);\n"
                    "}\n"
                    "void main() {\n"
                    "    vec4 c = sample2d(u_Texture0, v_TexCoords0, 0);\n"
                    "    c = u_c * c;\n");
        if (hasAlpha)
            frag.append("    c.a = 1.0; // before color mat transform!\n");
        if (isXYZ)
            frag.append("    c.rgb = pow(c.rgb, vec3(2.6));\n");
        frag.append("    c = u_colorMatrix * c;\n");
        if (isXYZ)
            frag.append("    c.rgb = pow(c.rgb, vec3(1.0/2.2));\n");
        frag.append("    gl_FragColor = clamp(c, 0.0, 1.0) * u_opacity;\n"
                    "}");

    }
    if (isTexRect) {
        frag.insert(0, "#extension GL_ARB_texture_rectangle : enable\n"
                       "#define sampler2D sampler2DRect\n");
        if (OpenglAide::GLSLVersion() < 140)
            frag.insert(0, "#undef texture\n"
                           "#define texture texture2DRect\n");
    }
    frag.insert(0, compatibleShaderHeader(GLShaderProgram::Fragment));
    AVDebug("fragmentShader: %s\n", frag.c_str());
    return frag;
}

#if 0 //Temporarily useless
static std::string& replace_distinct(std::string& src, const std::string& before, const std::string& after)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += after.length()) {
		if ((pos = src.find(before, pos)) != std::string::npos)
			src.replace(pos, before.length(), after);
		else break;
	}
	return src;
}
#endif

void VideoShader::initialize()
{
	DPTR_D(VideoShader);
	if (textureLocationCount() <= 0)
		return;
	if (!d->program) {
		d->program = new GLShaderProgram();
	}
	if (!d->program->isLinked()) {
		buildShader();
	}
    d->u_Matrix = d->program->uniformLocation("u_Matrix");
    //fragment shader
	d->u_colorMatrix = d->program->uniformLocation("u_colorMatrix");
    d->u_to8 = d->program->uniformLocation("u_to8");
    d->u_opacity = d->program->uniformLocation("u_opacity");
    d->u_c = d->program->uniformLocation("u_c");
	d->u_texelSize = d->program->uniformLocation("u_texelSize");
    d->u_textureSize = d->program->uniformLocation("u_textureSize");
    d->u_Texture.resize(textureLocationCount());
    AVDebug("uniform locations:\n");
    for (unsigned i = 0; i < d->u_Texture.size(); ++i) {
        std::string tex_var = "u_Texture" + std::to_string(i);
        d->u_Texture[i] = d->program->uniformLocation(tex_var.c_str());
        AVDebug("%s: %d\n", tex_var.c_str(), d->u_Texture[i]);
    }
	AVDebug("u_Matrix: %d\n", d->u_Matrix);
	AVDebug("u_colorMatrix: %d\n", d->u_colorMatrix);
	AVDebug("u_opacity: %d\n", d->u_opacity);
	if (d->u_c >= 0)
		AVDebug("u_c: %d\n", d->u_c);
	if (d->u_to8 >= 0)
		AVDebug("u_to8: %d\n", d->u_to8);
	if (d->u_texelSize >= 0)
		AVDebug("u_texelSize: %d\n", d->u_texelSize);
	if (d->u_textureSize >= 0)
		AVDebug("u_textureSize: %d\n", d->u_textureSize);

	//d->user_uniforms[VertexShader].clear();
	//d->user_uniforms[FragmentShader].clear();
	//if (userShaderHeader(QOpenGLShader::Vertex)) {
	//	tDebug("user uniform locations in vertex shader:");
	//	d->user_uniforms[VertexShader] = ParseUniforms(QByteArray(userShaderHeader(QOpenGLShader::Vertex)), d->program->programId());
	//}
	//if (userShaderHeader(QOpenGLShader::Fragment)) {
	//	tDebug("user uniform locations in fragment shader:");
	//	d->user_uniforms[FragmentShader] = ParseUniforms(QByteArray(userShaderHeader(QOpenGLShader::Fragment)), d->program->programId());
	//}
	d->rebuild_program = false;
	d->update_builtin_uniforms = true;
	//programReady(); // program and uniforms are ready
}

bool VideoShader::buildShader()
{
	DPTR_D(VideoShader);
	if (d->program->isLinked()) {
		AVWarning("Shader program is already linked");
	}
	d->program->removeAllShaders();
    d->program->addShaderFromSourceCode(GLShaderProgram::Vertex, vertexShader().c_str());
    d->program->addShaderFromSourceCode(GLShaderProgram::Fragment, fragmentShader().c_str());
	int maxVertexAttribs = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
	char const *const *attr = attributeNames();
	for (int i = 0; attr[i]; ++i) {
		if (i >= maxVertexAttribs) {
			AVError("List of attribute names is either too long or not null-terminated.\n"
				"Maximum number of attributes on this hardware is %i.\n"
				"Vertex shader:\n%s\n"
				"Fragment shader:\n%s\n",
                maxVertexAttribs, vertexShader().c_str(), fragmentShader().c_str());
		}
		// why must min location == 0?
		if (*attr[i]) {
			d->program->bindAttributeLocation(attr[i], i);
			AVDebug("bind attribute: %s => %d.\n", attr[i], i);
		}
	}

	if (!d->program->link()) {
		return false;
	}
	return true;
}

bool VideoShader::update(VideoMaterial *material)
{
	DPTR_D(VideoShader);
	const int mt = material->type();
	if (mt != d->material_type || d->rebuild_program) {
		// TODO: use shader program cache (per shader), check shader type
		AVDebug("Rebuild shader program requested: %d. Material type %d=>%d", d->rebuild_program, d->material_type, mt);
		d->program->removeAllShaders(); //not linked
		setVideoFormat(material->currentFormat());
		setTextureTarget(material->textureTarget());
		setMaterialType(material->type());
		initialize();
	}
	//material->unbind();
	const VideoFormat fmt(material->currentFormat()); //FIXME: maybe changed in setCurrentFrame(
	//format is out of date because we may use the same shader for different formats
	setVideoFormat(fmt);
	// uniforms begin
	d->program->bind(); //glUseProgram(id). for glUniform
	//if (!setUserUniformValues()) {
	//	if (!d->user_uniforms[VertexShader].isEmpty()) {
	//		for (int i = 0; i < d->user_uniforms[VertexShader].size(); ++i) {
	//			Uniform& u = d->user_uniforms[VertexShader][i];
	//			setUserUniformValue(u);
	//			if (u.dirty)
	//				u.setGL();
	//		}
	//	}
	//	if (!d->user_uniforms[FragmentShader].isEmpty()) {
	//		for (int i = 0; i < d->user_uniforms[FragmentShader].size(); ++i) {
	//			Uniform& u = d->user_uniforms[FragmentShader][i];
	//			setUserUniformValue(u);
	//			if (u.dirty)
	//				u.setGL();
	//		}
	//	}
	//}
	// shader type changed, eq mat changed, or other material properties changed (e.g. texture, 8bit=>10bit)
	if (!d->update_builtin_uniforms && !material->isDirty())
		return true;
	d->update_builtin_uniforms = false;
	// all texture ids should be binded when renderering even for packed plane!
	const int nb_planes = fmt.planeCount(); //number of texture id
	// TODO: sample2D array
	for (int i = 0; i < nb_planes; ++i) {
		// use glUniform1i to swap planes. swap uv: i => (3-i)%3
		d->program->setUniformValue(textureLocation(i), (GLint)i);
	}
	if (nb_planes < textureLocationCount()) {
		for (int i = nb_planes; i < textureLocationCount(); ++i) {
			d->program->setUniformValue(textureLocation(i), (GLint)(nb_planes - 1));
		}
	}
	d->program->setUniformValue(colorMatrixLocation(), material->colorMatrix());
	d->program->setUniformValue(opacityLocation(), (GLfloat)1.0);
	if (d->u_to8 >= 0)
		d->program->setUniformValue(d->u_to8, material->vectorTo8bit());
	if (channelMapLocation() >= 0)
		d->program->setUniformValue(channelMapLocation(), material->channelMap());
	//d->program->setUniformValue(matrixLocation(), ); //what about sgnode? state.combindMatrix()?
	if (texelSizeLocation() >= 0)
		d->program->setUniformValueArray(texelSizeLocation(), material->texelSize().data(), nb_planes);
	if (textureSizeLocation() >= 0)
		d->program->setUniformValueArray(textureSizeLocation(), material->textureSize().data(), nb_planes);
	// uniform end. attribute begins
	return true;
}

GLShaderProgram * VideoShader::program()
{
	return d_func()->program;
}

VideoFormat VideoShader::videoFormat() const
{
	DPTR_D(const VideoShader);
	return d->video_format;
}

void VideoShader::setVideoFormat(const VideoFormat &format)
{
	DPTR_D(VideoShader);
	d->video_format = format;
}

int VideoShader::textureTarget() const
{
	DPTR_D(const VideoShader);
	return d->texture_target;
}

void VideoShader::setTextureTarget(int type)
{
	DPTR_D(VideoShader);
	d->texture_target = type;
}

void VideoShader::setMaterialType(int value)
{
	DPTR_D(VideoShader);
	d->material_type = value;
}

int VideoShader::matrixLocation() const
{
	return d_func()->u_Matrix;
}

int VideoShader::textureLocationCount() const
{
	DPTR_D(const VideoShader);
	if (!d->video_format.isPlanar())
		return 1;
	return d->video_format.channels();
}

int VideoShader::textureLocation(int channel) const
{
	DPTR_D(const VideoShader);
	//Q_ASSERT(channel < d->u_Texture.size());
	return d->u_Texture[channel];
}

int VideoShader::colorMatrixLocation() const
{
	return d_func()->u_colorMatrix;
}

int VideoShader::opacityLocation() const
{
	return d_func()->u_opacity;
}

int VideoShader::channelMapLocation() const
{
	return d_func()->u_c;
}

int VideoShader::texelSizeLocation() const
{
	return d_func()->u_texelSize;
}

int VideoShader::textureSizeLocation() const
{
	return d_func()->u_textureSize;
}

NAMESPACE_END
