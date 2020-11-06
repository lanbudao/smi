#include "VideoFrame.h"
#include "Frame_p.h"

NAMESPACE_BEGIN

class VideoFramePrivate : public FramePrivate {
public:
    VideoFramePrivate():
        width(0),
        height(0),
        color_space(ColorSpace_Unknown),
        color_range(ColorRange_Unknown),
        displayAspectRatio(1.0),
        duration(0)
    {

    }

    void setFormat(const VideoFormat &fmt)
    {
        format = fmt;
        planes.reserve(fmt.planeCount());
        planes.resize(fmt.planeCount());
        line_sizes.reserve(fmt.planeCount());
        line_sizes.resize(fmt.planeCount());
    }
    VideoFormat format;
    int width, height;

    ColorSpace color_space;
    ColorRange color_range;

    float displayAspectRatio;
    double duration;
    double pkt_pts;
};

VideoFrame::VideoFrame():
    Frame(new VideoFramePrivate)
{

}

VideoFrame::VideoFrame(const VideoFrame &other):
    Frame(other)
{

}

VideoFrame::VideoFrame(int width, int height, const VideoFormat &format, const ByteArray &data):
    Frame(new VideoFramePrivate)
{
    DPTR_D(VideoFrame);
    d->width = width;
    d->height = height;
    d->data = data;
    d->setFormat(format);
}

VideoFrame::~VideoFrame()
{

}

VideoFrame &VideoFrame::operator=(const VideoFrame &other)
{
    d_ptr = other.d_ptr;
    return *this;
}

void VideoFrame::setData(void *data)
{
    DPTR_D(Frame);
    AVFrame* f = reinterpret_cast<AVFrame*>(data);
    av_frame_move_ref(d->frame, f);
    setBits(d->frame->data);
    setBytesPerLine(d->frame->linesize);
    //setTimestamp(static_cast<double>(d->frame->pts) / 1000.0);
    setPos(d->frame->pkt_pos);
}

int VideoFrame::channelCount() const {
    DPTR_D(const VideoFrame);
    if (!d->format.isValid())
        return 0;
    return d->format.channels();
}

bool VideoFrame::isValid() const {
    DPTR_D(const VideoFrame);
    return d->width > 0 && d->height > 0 && d->format.isValid();
}

Size VideoFrame::size() const{
    DPTR_D(const VideoFrame);
    return Size(d->width, d->height);
}

VideoFormat VideoFrame::format() const {
    DPTR_D(const VideoFrame);
    return d->format;
}

VideoFormat::PixelFormat VideoFrame::pixelFormat() const {
    DPTR_D(const VideoFrame);
    return d->format.pixelFormat();
}

int VideoFrame::pixelFormatFFmpeg() const {
    DPTR_D(const VideoFrame);
    return d->format.pixelFormatFFmpeg();
}

int VideoFrame::width() const {
    DPTR_D(const VideoFrame);
    return d->width;
}

int VideoFrame::height() const {
    DPTR_D(const VideoFrame);
    return d->height;
}

int VideoFrame::effectiveBytesPerLine(int plane) const
{
	DPTR_D(const VideoFrame);
    return d->format.bytesPerLine(width(), plane);
}

void VideoFrame::setDuration(double duration)
{
    DPTR_D(VideoFrame);
    d->duration = duration;
}

double VideoFrame::duration() const
{
    return d_func()->duration;
}

ColorSpace VideoFrame::colorSpace() const {
    DPTR_D(const VideoFrame);
    return d->color_space;
}

void VideoFrame::setColorSpace(ColorSpace space) {
    DPTR_D(VideoFrame);
    d->color_space = space;
}

ColorRange VideoFrame::colorRange() const {
    DPTR_D(const VideoFrame);
    return d->color_range;
}

void VideoFrame::setColorRange(ColorRange range) {
    DPTR_D(VideoFrame);
    d->color_range = range;
}

int VideoFrame::planeWidth(int plane) const {
    DPTR_D(const VideoFrame);
    return d->format.width(d->width, plane);
}

int VideoFrame::planeHeight(int plane) const {
    DPTR_D(const VideoFrame);
    return d->format.height(d->height, plane);
}

void VideoFrame::setDisplayAspectRatio(float aspect) {
    DPTR_D(VideoFrame);
    d->displayAspectRatio = aspect;
}

float VideoFrame::displayAspectRatio() const
{
	DPTR_D(const VideoFrame);
	if (d->displayAspectRatio > 0)
		return d->displayAspectRatio;

	if (d->width > 0 && d->height > 0)
        return FORCE_FLOAT(d->width) / FORCE_FLOAT(d->height);

	return 0;
}

void *VideoFrame::map(SurfaceType type, void *handle, int plane)
{
	return map(type, handle, format(), plane);
}

void *VideoFrame::map(SurfaceType type, void *handle, const VideoFormat& fmt, int plane)
{
	DPTR_D(VideoFrame);
	//const ByteArray v = d->metadata.at("surface_interop");
	//if (!v.isEmpty())
	//	return 0;
	//d->surface_interop = v.value<VideoSurfaceInteropPtr>();
	//if (!d->surface_interop)
	//	return 0;
	//if (plane > planeCount())
	//	return 0;
	//return d->surface_interop->map(type, fmt, handle, plane);
	return 0;
}

void VideoFrame::unmap(void *handle)
{
	//DPTR_D(VideoFrame);
	//if (!d->surface_interop)
	//	return;
	//d->surface_interop->unmap(handle);
}

void* VideoFrame::createInteropHandle(void* handle, SurfaceType type, int plane)
{
	DPTR_D(VideoFrame);
	//const ByteArray v = d->metadata.at("surface_interop");
	//if (!v.isEmpty())
	//	return 0;
	//d->surface_interop = v.value<VideoSurfaceInteropPtr>();
	//if (!d->surface_interop)
	//	return 0;
	//if (plane > planeCount())
	//	return 0;
	//return d->surface_interop->createHandle(handle, type, format(), 
	//	plane, planeWidth(plane), planeHeight(plane));
    return 0;
}

double VideoFrame::ptsOfPkt() const
{
    return d_func()->pkt_pts;
}

void VideoFrame::setPtsOfPkt(double pts)
{
    d_func()->pkt_pts = pts;
}

NAMESPACE_END
