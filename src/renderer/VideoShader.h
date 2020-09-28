#ifndef VIDEO_SHADER_H
#define VIDEO_SHADER_H

#include "sdk/DPTR.h"
#include "VideoFrame.h"

NAMESPACE_BEGIN

/**
 * @brief The VideoShader class
 * Encapsulates rendering state for a video shader program.
 * Low-level api. Used by OpenGLVideo and ShaderManager
 */
class VideoMaterial;
class GLShaderProgram;
class VideoShaderPrivate;
class VideoShader
{
	DPTR_DECLARE_PRIVATE(VideoShader);
public:
	VideoShader();
	~VideoShader();

	std::string shaderSourceFromFile(const std::string &fileName) const;
	/**
     * @brief attributeNames
     * Array must end with null. { position, texcoord, ..., 0}, location is bound to 0, 1, ...
     * @return
     */
    virtual char const *const *attributeNames() const;
	/**
	 * @brief vertexShader
	 * mvp uniform: u_Matrix
	 * Vertex shader in: a_Position, a_TexCoordsN (see attributeNames())
	 * Vertex shader out: v_TexCoordsN
	 */
	virtual std::string vertexShader() const;
	virtual std::string fragmentShader() const;
    virtual void initialize();
	bool buildShader();
	bool update(VideoMaterial *material);
	GLShaderProgram* program();

	VideoFormat videoFormat() const;
	void setVideoFormat(const VideoFormat& format);
	int textureTarget() const;
	void setTextureTarget(int type);
	void setMaterialType(int value);
	int matrixLocation() const;
	int textureLocationCount() const;
	int textureLocation(int channel) const;
	int colorMatrixLocation() const;
	int opacityLocation() const;
	int channelMapLocation() const;
	int texelSizeLocation() const;
	int textureSizeLocation() const;

private:
	DPTR_DECLARE(VideoShader);
};

NAMESPACE_END
#endif //VIDEO_SHADER_H
