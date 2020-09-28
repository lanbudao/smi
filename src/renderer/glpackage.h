#ifndef GLPACKAGE_H
#define GLPACKAGE_H

#include "sdk/DPTR.h"
#include "VectorF.h"
#include "Matrix4x4.h"
#include "OpenglAide.h"

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;
typedef char GLchar;

NAMESPACE_BEGIN

class GLBufferPrivate;
class GLBuffer
{
	DPTR_DECLARE_PRIVATE(GLBuffer);
public:
	enum Type
	{
		VertexBuffer = 0x8892, // GL_ARRAY_BUFFER
		IndexBuffer = 0x8893, // GL_ELEMENT_ARRAY_BUFFER
		PixelPackBuffer = 0x88EB, // GL_PIXEL_PACK_BUFFER
		PixelUnpackBuffer = 0x88EC  // GL_PIXEL_UNPACK_BUFFER
	};
	enum Access
	{
		ReadOnly = 0x88B8, // GL_READ_ONLY
		WriteOnly = 0x88B9, // GL_WRITE_ONLY
		ReadWrite = 0x88BA  // GL_READ_WRITE
	};

	enum RangeAccessFlag
	{
		RangeRead = 0x0001, // GL_MAP_READ_BIT
		RangeWrite = 0x0002, // GL_MAP_WRITE_BIT
		RangeInvalidate = 0x0004, // GL_MAP_INVALIDATE_RANGE_BIT
		RangeInvalidateBuffer = 0x0008, // GL_MAP_INVALIDATE_BUFFER_BIT
		RangeFlushExplicit = 0x0010, // GL_MAP_FLUSH_EXPLICIT_BIT
		RangeUnsynchronized = 0x0020  // GL_MAP_UNSYNCHRONIZED_BIT
	};
    GLBuffer(GLBuffer::Type t = VertexBuffer);
    ~GLBuffer();

	bool create();
	bool isCreated() const ;
	GLuint id() const;
    bool bind();
	void unbind();
    void allocate(int dataSize);
    void allocate(const void *data, int dataSize);
    void write(int offset, const void *data, int count);
	void destroy();
	int size() const;

	void *map(GLBuffer::Access access);
	void *mapRange(int offset, int count, GLBuffer::RangeAccessFlag access);
	bool unmap();


private:
	DPTR_DECLARE(GLBuffer);
};

class GLArray
{
public:
    GLArray();
    ~GLArray();

	GLuint id() { return _id; }

	bool create();
	bool isCreated() const;
    void bind();
	void unbind();
	void destroy();

private:
    GLuint _id;
};

class GLShaderProgramPrivate;
class GLShaderProgram
{
	DPTR_DECLARE_PRIVATE(GLShaderProgram);
public:
    typedef enum ShaderTypeBit
    {
        Vertex = 0x0001,
        Fragment = 0x0002,
        Geometry = 0x0004,
        TessellationControl = 0x0008,
        TessellationEvaluation = 0x0010,
        Compute = 0x0020
    } ShaderType;
    GLShaderProgram();
    ~GLShaderProgram();

    bool addShaderFromSourceCode(ShaderType shaderType, const char* src);
	void removeAllShaders();
    void bindAttributeLocation(const GLchar* name, GLuint index);
    bool link();
	bool isLinked() const;
    void bind();
    void unBind();
    GLint attribLocation(const GLchar* name);
	GLint uniformLocation(const GLchar *name);

	void setUniformValue(int location, GLfloat value);
	void setUniformValue(int location, GLint value);
    void setUniformValue(int location, GLdouble value);
	void setUniformValue(int location, const Vector2D& value);
	void setUniformValueArray(int location, const Vector2D *values, int count);
	void setUniformValue(int location, const Matrix4x4& value);
	bool isValid() const;

private:
	DPTR_DECLARE(GLShaderProgram);
};

class GLContext
{
 public:
    GLContext();
    ~GLContext();
};

NAMESPACE_END
#endif // GLPACKAGE_H
