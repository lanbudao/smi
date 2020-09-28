#ifndef VIDEO_MATERIAL_H
#define VIDEO_MATERIAL_H

#include "sdk/DPTR.h"
#include "VideoFrame.h"
#include "Matrix4x4.h"
#include "VectorF.h"
#include "RectF.h"

NAMESPACE_BEGIN

/**
 * @brief The VideoMaterial class
 * Encapsulates rendering state for a video shader program.
 * Low-level api. Used by OpenGLVideo and Scene Graph
 */
//class VideoShader;
class VideoMaterialPrivate;
class VideoMaterial
{
	DPTR_DECLARE_PRIVATE(VideoMaterial)
public:
	VideoMaterial();
	virtual ~VideoMaterial() {}

	void setCurrentFrame(const VideoFrame& frame);
	VideoFormat currentFormat() const;
	int textureTarget() const;
	//VideoShader* createShader() const;
	virtual int type() const;
	static std::string typeName(int value);

	bool bind(); // TODO: roi
	void unbind();
	void setDirty(bool value);    /*!
     * \brief isDirty
     * \return true if material type changed, or other properties changed, e.g. 8bit=>10bit (the same material type) and eq
     */
    bool isDirty() const;

	const Matrix4x4 &colorMatrix() const;
	const Matrix4x4 &channelMap() const;
	Vector2D vectorTo8bit() const;

	double validTextureWidth() const;
	Size frameSize() const;
	 /*!
     * \brief texelSize
     * The size of texture unit
     * \return (1.0/textureWidth, 1.0/textureHeight)
     */
    SizeF texelSize(int plane) const; //vec2?
    /*!
     * \brief texelSize
     * For GLSL. 1 for rectangle texture, 1/(width, height) for 2d texture
     */
    std::vector<Vector2D> texelSize() const;
    /*!
     * \brief textureSize
     * It can be used with a uniform to emulate GLSL textureSize() which exists in new versions.
     */
    Size textureSize(int plane) const;
    /*!
     * \brief textureSize
     * For GLSL. Not normalized
     */
    std::vector<Vector2D> textureSize() const;
	double brightness() const;
	void setBrightness(double value);
	double contrast() const;
	void setContrast(double value);
	double hue() const;
	void setHue(double value);
	double saturation() const;
	void setSaturation(double value);

	RectF mapToTexture(int plane, const RectF& r, int normalize = -1) const;

private:
	DPTR_DECLARE(VideoMaterial);
};
NAMESPACE_END
#endif //VIDEO_MATERIAL_H

