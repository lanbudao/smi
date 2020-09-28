#include "glpackage.h"
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include "AVLog.h"

NAMESPACE_BEGIN

class GLBufferPrivate
{
public:
	GLBufferPrivate() :
		id(0)
	{

	}
	GLenum type;
	GLuint id;
};

GLBuffer::GLBuffer(GLBuffer::Type t):
	d_ptr(new GLBufferPrivate)
{
	DPTR_D(GLBuffer);
	d->type = t;
}

GLBuffer::~GLBuffer()
{
	destroy();
}

bool GLBuffer::create()
{
	DPTR_D(GLBuffer);
	glGenBuffers(1, &d->id);
	return d->id != 0;
}

bool GLBuffer::isCreated() const
{
	DPTR_D(const GLBuffer);
	return d->id != 0;
}

GLuint GLBuffer::id() const
{
	DPTR_D(const GLBuffer);
	return d->id;
}

bool GLBuffer::bind()
{
	DPTR_D(GLBuffer);
    if (d->id == 0)
        return false;
    glBindBuffer(d->type, d->id);
    return true;
}

void GLBuffer::unbind()
{
    DPTR_D(GLBuffer);
    glBindBuffer(d->type, 0);
}

void GLBuffer::allocate(int dataSize)
{
	DPTR_D(GLBuffer);
    glBufferData(d->type, dataSize, nullptr, GL_STATIC_DRAW);
}

void GLBuffer::allocate(const void *data, int dataSize)
{
	DPTR_D(GLBuffer);
    glBufferData(d->type, dataSize, data, GL_STATIC_DRAW);
}

void GLBuffer::write(int offset, const void *data, int count)
{
	DPTR_D(GLBuffer);
    glBufferSubData(d->type, offset, count, data);
}

void GLBuffer::destroy()
{
	DPTR_D(GLBuffer);
    glDeleteBuffers(1, &d->id);
}
/*!
	Returns the size of the data in this buffer, for reading operations.
	Returns -1 if fetching the buffer size is not supported, or the
	buffer has not been created.

	It is assumed that this buffer has been bound to the current context.

	\sa isCreated(), bind()
*/
int GLBuffer::size() const
{
	DPTR_D(const GLBuffer);
	if (!d->id)
		return -1;
	GLint value = -1;
	glGetBufferParameteriv(d->type, GL_BUFFER_SIZE, &value);
	return value;
}

void* GLBuffer::map(GLBuffer::Access access)
{
	DPTR_D(GLBuffer);
    if (true/*d->funcs->hasOpenGLExtension(QOpenGLExtensions::MapBufferRange)*/) {
		GLBuffer::RangeAccessFlag rangeAccess;
		switch (access) {
		case GLBuffer::ReadOnly:
			rangeAccess = GLBuffer::RangeRead;
			break;
		case GLBuffer::WriteOnly:
			rangeAccess = GLBuffer::RangeWrite;
			break;
		case GLBuffer::ReadWrite:
			rangeAccess = static_cast<GLBuffer::RangeAccessFlag>(GLBuffer::RangeRead | GLBuffer::RangeWrite);
			break;
		}
		return glMapBufferRange(d->type, 0, size(), rangeAccess);
	}
	else {
		return glMapBuffer(d->type, access);
	}
}
/*!
	Maps the range specified by \a offset and \a count of the contents
	of this buffer into the application's memory space and returns a
	pointer to it.  Returns null if memory mapping is not possible.
	The \a access parameter specifies a combination of access flags.

	It is assumed that create() has been called on this buffer and that
	it has been bound to the current context.

	\note This function is not available on OpenGL ES 2.0 and earlier.

	\sa unmap(), create(), bind()
 */
void *GLBuffer::mapRange(int offset, int count, GLBuffer::RangeAccessFlag access)
{
	DPTR_D(GLBuffer);
	if (!isCreated())
		AVWarning("GLBuffer::mapRange(): buffer not created\n");
	if (!d->id)
		return nullptr;
	return glMapBufferRange(d->type, offset, count, access);
}

bool GLBuffer::unmap()
{
	DPTR_D(GLBuffer);
	return glUnmapBuffer(d->type) == GL_TRUE;
}

class GLShaderProgramPrivate
{
public:
	GLShaderProgramPrivate():
		islinked(false)
	{
		shaders.clear();
	}

	~GLShaderProgramPrivate()
	{
		removeAllShaders();
		glDeleteProgram(program);
	}

	void removeAllShaders()
	{
		for (std::vector<GLenum>::iterator it = shaders.begin(); it != shaders.end(); it++) {
			glDeleteShader(static_cast<GLenum>(*it));
		}
	}

	GLuint program;
	bool islinked;
	std::vector<GLenum> shaders;
};

GLShaderProgram::GLShaderProgram():
	d_ptr(new GLShaderProgramPrivate)
{
	DPTR_D(GLShaderProgram);
    d->program = glCreateProgram();
}

GLShaderProgram::~GLShaderProgram()
{
}


static unsigned int getShaderTypeValue(GLShaderProgram::ShaderType &type)
{
    GLenum value;
    switch (type)
    {
    case GLShaderProgram::Vertex:
        value = GL_VERTEX_SHADER;
        break;
    case GLShaderProgram::Fragment:
        value =  GL_FRAGMENT_SHADER;
        break;
    case GLShaderProgram::Geometry:
        value =  GL_GEOMETRY_SHADER;
        break;
    case GLShaderProgram::TessellationControl:
        value =  GL_TESS_CONTROL_SHADER;
        break;
    case GLShaderProgram::TessellationEvaluation:
        value =  GL_TESS_EVALUATION_SHADER;
        break;
    case GLShaderProgram::Compute:
        value =  GL_COMPUTE_SHADER;
        break;
    }
    return value;
}

bool GLShaderProgram::addShaderFromSourceCode(ShaderType shaderType, const char *src)
{
	DPTR_D(GLShaderProgram);
    GLint ret;
	GLuint shader;
	GLsizei len;

    shader = glCreateShader(getShaderTypeValue(shaderType));
	if (!glIsShader(shader))
		return false;
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
	if (ret != GL_TRUE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len <= 0) {
            glDeleteShader(shader);
            return false;
        }
		std::shared_ptr<GLchar> log(
            new GLchar[len + 1], std::default_delete<GLchar>());
		glGetShaderInfoLog(shader, len, &len, log.get());
		AVWarning("Failed to compile the shader: %s.\n", log.get());
        glDeleteShader(shader);
		return false;
    }
    glAttachShader(d->program, shader);
	d->shaders.push_back(shader);
    return true;
}

void GLShaderProgram::removeAllShaders()
{
	DPTR_D(GLShaderProgram);
	d->removeAllShaders();
}

void GLShaderProgram::bindAttributeLocation(const GLchar* name, GLuint index)
{
	DPTR_D(GLShaderProgram);
    glBindAttribLocation(d->program, index, name);
}

bool GLShaderProgram::link()
{
	DPTR_D(GLShaderProgram);
    int ret;

    glLinkProgram(d->program);
    glGetProgramiv(d->program, GL_LINK_STATUS, &ret);
	if (ret == GL_FALSE) {
		GLsizei len;
		glGetProgramiv(d->program, GL_INFO_LOG_LENGTH, &len);
		std::shared_ptr<GLchar> log(
			new GLchar[len + 1], std::default_delete<GLchar>());
		glGetProgramInfoLog(d->program, len, &len, log.get());
		AVWarning("Program link failed: %s.\n", log.get());
		return false;
	}
	d->islinked = true;
    return true;
}

bool GLShaderProgram::isLinked() const
{
	DPTR_D(const GLShaderProgram);
	return d->islinked;
}

void GLShaderProgram::bind()
{
	DPTR_D(GLShaderProgram);
    glUseProgram(d->program);
}

void GLShaderProgram::unBind()
{
    glUseProgram(0);
}

GLint GLShaderProgram::attribLocation(const GLchar *name)
{
    DPTR_D(GLShaderProgram);
    return glGetAttribLocation(d->program, name);
}

GLint GLShaderProgram::uniformLocation(const GLchar *name)
{
	DPTR_D(GLShaderProgram);
    return glGetUniformLocation(d->program, name);
}

void GLShaderProgram::setUniformValue(int location, GLfloat value)
{
	glUniform1f(location, value);
}

void GLShaderProgram::setUniformValue(int location, GLint value)
{
    glUniform1i(location, value);
}

void GLShaderProgram::setUniformValue(int location, GLdouble value)
{
    glUniform1d(location, value);
}

void GLShaderProgram::setUniformValue(int location, const Vector2D & value)
{
	if (location != -1)
		glUniform2fv(location, 1, reinterpret_cast<const GLfloat *>(&value));
}

void GLShaderProgram::setUniformValueArray(int location, const Vector2D * values, int count)
{
	if (location != -1)
		glUniform2fv(location, count, reinterpret_cast<const GLfloat *>(values));
}

void GLShaderProgram::setUniformValue(int location, const Matrix4x4 & value)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, value.constData());
}

bool GLShaderProgram::isValid() const
{
	DPTR_D(const GLShaderProgram);
	return d->program != 0;
}

GLContext::GLContext()
{

}

GLContext::~GLContext()
{

}

GLArray::GLArray():
	_id(0)
{

}

GLArray::~GLArray()
{
	destroy();
}

bool GLArray::create()
{
	glGenVertexArrays(1, &_id);
	return _id != 0;
}

bool GLArray::isCreated() const
{
	return _id != 0;
}

void GLArray::bind()
{
    glBindVertexArray(_id);
}

void GLArray::unbind()
{
    glBindVertexArray(0);
}

void GLArray::destroy()
{
	glDeleteVertexArrays(1, &_id);
}

NAMESPACE_END
