#ifndef OPENGL_AIDE_H
#define OPENGL_AIDE_H

#include <stdint.h>
#include <string>
#include "glad/glad.h"
#include "sdk/global.h"

NAMESPACE_BEGIN

class VideoFormat;
class Matrix4x4;
namespace OpenglAide
{

typedef struct OpenglVersion {
	OpenglVersion(): major(0), minor(0), patch(0) {}
	uint8_t major = 4;
	uint8_t minor = 6;
	uint8_t patch = 0;
	bool isValid() {return major > 0 || minor > 0 || patch > 0;}
	std::string toString() {
		std::string s;
		s += std::to_string(major);
		s += std::to_string(minor);
		s += std::to_string(patch);
        return s;
	}
} OpenglVersion;

/**
 * current version: "4.6.0 - Build 26.20.100.7263"
 */
const OpenglVersion& getOpenglVersion();

int depth16BitTexture();

bool useDeprecatedFormats();

bool isOpenGLES();

bool hasRG();

bool has16BitTexture();

bool videoFormatToGL(const VideoFormat& fmt, GLint* internal_format, GLenum* data_format,
	GLenum* data_type, Matrix4x4* mat);

int bytesOfTexel(GLenum format, GLenum dataType);

int GLSLVersion();

bool isPBOSupported();

}
NAMESPACE_END
#endif // OPENGL_AIDE_H
