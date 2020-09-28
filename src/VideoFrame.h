#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include "VideoFormat.h"
#include "Frame.h"
#include "util/Size.h"

NAMESPACE_BEGIN

/*!
 * \brief The SurfaceType enum
 * HostMemorySurface:
 * Map the decoded frame to host memory
 * GLTextureSurface:
 * Map the decoded frame as an OpenGL texture
 * SourceSurface:
 * get the original surface from decoder, for example VASurfaceID for va-api, CUdeviceptr for CUDA and IDirect3DSurface9* for DXVA.
 * Zero copy mode is required.
 * UserSurface:
 * Do your own magic mapping with it
 */
enum SurfaceType {
    HostMemorySurface,
    GLTextureSurface,
    SourceSurface,
    UserSurface = 0xffff
};

class VideoFramePrivate;
class FFPRO_EXPORT VideoFrame: public Frame
{
    DPTR_DECLARE_PRIVATE(VideoFrame)
public:
    VideoFrame();
    VideoFrame(const VideoFrame &other);
    VideoFrame(int width, int height, const VideoFormat &format, const ByteArray &data = ByteArray());
    virtual ~VideoFrame();
    VideoFrame &operator=(const VideoFrame &other);

    void setData(void* data);

    int channelCount() const;

    VideoFormat format() const;

    VideoFormat::PixelFormat pixelFormat() const;
    int pixelFormatFFmpeg() const;

    bool isValid() const;

    Size size() const;
    int width() const;
    int height() const;
	int effectiveBytesPerLine(int plane) const;

    void setDuration(double d);
    double duration() const;

    ColorSpace colorSpace() const;
    void setColorSpace(ColorSpace space);

    ColorRange colorRange() const;
    void setColorRange(ColorRange range);

    int planeWidth(int plane) const;
    int planeHeight(int plane) const;

	void setDisplayAspectRatio(float aspect);
	float displayAspectRatio() const;
	    /*!
     * map a gpu frame to opengl texture or d3d texture or other handle.
     * handle: given handle. can be gl texture (& GLuint), d3d texture, or 0 if create a new handle
     * return the result handle or 0 if not supported
     */
    void* map(SurfaceType type, void* handle, int plane = 0);
    void* map(SurfaceType type, void* handle, const VideoFormat& fmt, int plane = 0);
    void unmap(void* handle);
    /*!
     * \brief createInteropHandle
     * \param handle input/output handle
     * \return null on error. otherwise return the input handle
     */
    void* createInteropHandle(void* handle, SurfaceType type, int plane);
    double ptsOfPkt() const;
    void setPtsOfPkt(double pts);
private:
};

NAMESPACE_END
#endif //VIDEOFRAME_H
