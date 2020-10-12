#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "AVDecoder.h"
#include "VideoFrame.h"
#include "Packet.h"
#include <vector>

NAMESPACE_BEGIN

typedef int VideoDecoderId;

class VideoDecoderPrivate;
class VideoDecoder: public AVDecoder
{
    DISABLE_COPY(VideoDecoder)
    DPTR_DECLARE_PRIVATE(VideoDecoder)
public:
    static std::vector<VideoDecoderId> registered();
    static StringList supportedCodecs();
    /**
     * @brief create
     * create a decoder from registered name. FFmpeg decoder will be created for empty name
     * @param name can be "FFmpeg", "CUDA", "VDA", "VAAPI", "DXVA", "Cedarv"
     * @return 0 if not registered
     */
    static VideoDecoder* create(const char* name = "FFmpeg");
    static VideoDecoder* create(VideoDecoderId id);

    virtual VideoDecoderId id() const = 0;
    virtual std::string name() const;
    virtual VideoFrame frame() = 0;
    virtual std::string description() const;

    template<class T>
    static bool Register(VideoDecoderId id, const char *name) { return Register(id, create<T>, name); }

    /**
     * @brief next
     * @param id NULL to get the first id address
     * @return address of id or NULL if not found/end
     */
    static VideoDecoderId* next(VideoDecoderId* id = 0);
    static const char* name(VideoDecoderId id);
    static VideoDecoderId id(const char* name);

    virtual int decode(const Packet& packet) = 0;

private:
    template<class T>
    static VideoDecoder * create() { return new T(); }
    typedef VideoDecoder* (*VideoDecoderCreator)();

protected:
    static bool Register(VideoDecoderId id, VideoDecoderCreator, const char *name);

protected:
    VideoDecoder(VideoDecoderPrivate *d);
};


extern FFPRO_EXPORT VideoDecoderId VideoDecoderId_FFmpeg;
extern FFPRO_EXPORT VideoDecoderId VideoDecoderId_MMAL;
extern FFPRO_EXPORT VideoDecoderId VideoDecoderId_QSV;
extern FFPRO_EXPORT VideoDecoderId VideoDecoderId_CrystalHD;

NAMESPACE_END
#endif //VIDEODECODER_H
