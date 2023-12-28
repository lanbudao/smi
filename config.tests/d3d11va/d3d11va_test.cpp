//#include "../dxcompat.h"
#include <d3d11.h>
extern "C" {
#include <libavcodec/d3d11va.h> 
}
#include <wrl/client.h> //ComPtr is used in QtAV

int main()
{
    av_d3d11va_alloc_context();
    D3D11_VIDEO_PROCESSOR_STREAM s; //used by vp
    return 0;
}
