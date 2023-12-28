#include "directx/videodecoderd3d.h"
#include "directx/videodecoderd3d_p.h"
#include "utils/factory.h"
#include "utils/AVLog.h"

// d3d9ex: http://dxr.mozilla.org/mozilla-central/source/dom/media/wmf/DXVA2Manager.cpp

//#define DXVA2API_USE_BITFIELDS
extern "C" {
#include <libavcodec/dxva2.h>
}
#include <d3d9.h>
#include <dxva2api.h>

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#ifdef __MINGW32__
# include <_mingw.h>
# if !defined(__MINGW64_VERSION_MAJOR)
#  undef MS_GUID
#  define MS_GUID DEFINE_GUID /* dxva2api.h fails to declare those, redefine as static */
# else
#  include <dxva.h>
# endif
#endif /* __MINGW32__ */

NAMESPACE_BEGIN

class VideoDecoderD3D11VAPrivate final: public VideoDecoderD3DPrivate
{
public:
    VideoDecoderD3D11VAPrivate()
    {

    }
    virtual ~VideoDecoderD3D11VAPrivate()
    {
    }

public:

};
class VideoDecoderD3D11VA : public VideoDecoderD3D
{
    DPTR_DECLARE_PRIVATE(VideoDecoderD3D11VA)
public:
    VideoDecoderD3D11VA(VideoDecoderD3D11VAPrivate *d_ptr = nullptr):
        VideoDecoderD3D(d_ptr)
    {

    }
    VideoDecoderId id() const override;
    std::string description() const override;
    VideoFrame frame() override;
};

VideoDecoderId VideoDecoderD3D11VA::id() const
{

}

string VideoDecoderD3D11VA::description() const
{

}

VideoFrame VideoDecoderD3D11VA::frame()
{

}

NAMESPACE_END
