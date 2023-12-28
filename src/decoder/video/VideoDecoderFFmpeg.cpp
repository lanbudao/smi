#include "VideoDecoderFFmpegBase.h"
#include "private/VideoDecoderFFmpegBase_p.h"
#include "Factory.h"
#include "AVDecoder_p.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN

class VideoDecoderFFmpegPrivate;
class VideoDecoderFFmpeg: public VideoDecoderFFmpegBase
{
public:
    enum StrictType {
        Very = FF_COMPLIANCE_VERY_STRICT,
        Strict = FF_COMPLIANCE_STRICT,
        Normal = FF_COMPLIANCE_NORMAL, //default
        Unofficial = FF_COMPLIANCE_UNOFFICIAL,
        Experimental = FF_COMPLIANCE_EXPERIMENTAL
    };

    enum DiscardType {
        None = AVDISCARD_NONE,
        Default = AVDISCARD_DEFAULT, //default
        NoRef = AVDISCARD_NONREF,
        Bidir = AVDISCARD_BIDIR,
        NoKey = AVDISCARD_NONKEY,
        All = AVDISCARD_ALL
    };

    enum ThreadFlag {
        DefaultType = FF_THREAD_SLICE | FF_THREAD_FRAME,//default
        Slice = FF_THREAD_SLICE,
        Frame = FF_THREAD_FRAME
    };
    // flags. visualize motion vectors (MVs)
    enum MotionVectorVisFlag {
        No = 0, //default
        PF = FF_DEBUG_VIS_MV_P_FOR,
        BF = FF_DEBUG_VIS_MV_B_FOR,
        BB = FF_DEBUG_VIS_MV_B_BACK
    };
    enum BugFlag {
        autodetect = FF_BUG_AUTODETECT, //default
        xvid_ilace = FF_BUG_XVID_ILACE,
        ump4 = FF_BUG_UMP4,
        no_padding = FF_BUG_NO_PADDING,
        amv = FF_BUG_AMV,
        qpel_chroma = FF_BUG_QPEL_CHROMA,
        std_qpel = FF_BUG_QPEL_CHROMA2,
        direct_blocksize = FF_BUG_DIRECT_BLOCKSIZE,
        edge = FF_BUG_EDGE,
        hpel_chroma = FF_BUG_HPEL_CHROMA,
        dc_clip = FF_BUG_DC_CLIP,
        ms = FF_BUG_MS,
        trunc = FF_BUG_TRUNCATED
    };

    DPTR_DECLARE_PRIVATE(VideoDecoderFFmpeg)
    VideoDecoderFFmpeg();
    ~VideoDecoderFFmpeg();

    static VideoDecoder* createMMAL() {
        VideoDecoderFFmpeg *vd = new VideoDecoderFFmpeg();
//        vd->setProperty("hwaccel", "mmal");
        return vd;
    }

    static VideoDecoder* createQSV() {
        VideoDecoderFFmpeg *vd = new VideoDecoderFFmpeg();
//        vd->setProperty("hwaccel", "qsv");
        return vd;
    }

    static VideoDecoder* createCrystalHD() {
        VideoDecoderFFmpeg *vd = new VideoDecoderFFmpeg();
//        vd->setProperty("hwaccel", "crystalhd");
        return vd;
    }

    static void registerHWA()
    {
        VideoDecoder::Register(VideoDecoderId_QSV, createQSV, "QSV");
    }

    VideoDecoderId id() const PU_DECL_OVERRIDE;
    std::string description() const PU_DECL_OVERRIDE;

    void setSkipLoopFilter(DiscardType value);
    DiscardType skipLoopFilter() const;
    void setSkipIDCT(DiscardType value);
    DiscardType skipIDCT() const;
    void setStrict(StrictType value);
    StrictType strict() const;
    void setSkipFrame(DiscardType value);
    DiscardType skipFrame() const;
    void setThreads(int value);
    int threads() const;
    void setThreadFlags(ThreadFlag value);
    ThreadFlag threadFlags() const;
    void setMotionVectorVisFlags(MotionVectorVisFlag value);
    MotionVectorVisFlag motionVectorVisFlags() const;
    void setBugFlags(BugFlag value);
    BugFlag bugFlags() const;
    void setHwaccel(const std::string& value);
    std::string hwaccel() const;
};

extern VideoDecoderId VideoDecoderId_FFmpeg;

FACTORY_REGISTER(VideoDecoder, FFmpeg, "FFmpeg")

void RegisterFFmpegHWA_Man()
{
    VideoDecoderFFmpeg::registerHWA();
}
namespace {
    static const struct factory_register_FFmpegHWA {
        inline factory_register_FFmpegHWA() {
            VideoDecoderFFmpeg::registerHWA();
        }
    } sInit_FFmpegHWA;
}

class VideoDecoderFFmpegPrivate: public VideoDecoderFFmpegBasePrivate
{
public:
    VideoDecoderFFmpegPrivate() :
        skip_loop_filter(VideoDecoderFFmpeg::Default),
        skip_idct(VideoDecoderFFmpeg::Default),
        strict(VideoDecoderFFmpeg::Normal),
        skip_frame(VideoDecoderFFmpeg::Default),
        thread_type(VideoDecoderFFmpeg::DefaultType),
        threads(0),
        vismv(VideoDecoderFFmpeg::No),
        bug(VideoDecoderFFmpeg::autodetect)
    {

    }
    ~VideoDecoderFFmpegPrivate()
    {

    }
    bool open() PU_DECL_OVERRIDE {
        av_opt_set_int(codec_ctx, "skip_loop_filter", (int64_t) skip_loop_filter, 0);
        av_opt_set_int(codec_ctx, "skip_idct", (int64_t) skip_idct, 0);
        av_opt_set_int(codec_ctx, "strict", (int64_t) strict, 0);
        av_opt_set_int(codec_ctx, "skip_frame", (int64_t) skip_frame, 0);
        av_opt_set_int(codec_ctx, "threads", (int64_t) threads, 0);
        av_opt_set_int(codec_ctx, "thread_type", (int64_t) thread_type, 0);
        av_opt_set_int(codec_ctx, "vismv", (int64_t) vismv, 0);
        av_opt_set_int(codec_ctx, "bug", (int64_t) bug, 0);
        return true;
    }

    int skip_loop_filter;
    int skip_idct;
    int strict;
    int skip_frame;
    int thread_type;
    int threads;
    int vismv;
    int bug;
    std::string hwa;
};

VideoDecoderFFmpeg::VideoDecoderFFmpeg()
    :VideoDecoderFFmpegBase(new VideoDecoderFFmpegPrivate())
{
//    setProperty("detail_skip_loop_filter", ("Skipping the loop filter (aka deblocking) usually has determinal effect on quality. However it provides a big speedup for hi definition streams"));
//    // like skip_frame
//    setProperty("detail_skip_idct", ("Force skipping of idct to speed up decoding for frame types (-1=None, "
//                                       "0=Default, 1=B-frames, 2=P-frames, 3=B+P frames, 4=all frames)"));
//    setProperty("detail_skip_frame", ("Force skipping frames for speed up decoding."));
//    setProperty("detail_threads", Util::sformat("%s\n%s\n%s\n",
//            "Number of decoding threads. Set before open. Maybe no effect for some decoders",
//            "0: auto",
//                                         "1: single thread decoding"));
}

VideoDecoderFFmpeg::~VideoDecoderFFmpeg()
{

}

VideoDecoderId VideoDecoderFFmpeg::id() const
{
    DPTR_D(const VideoDecoderFFmpeg);
    if (d->hwaccel == "mmal")
        return VideoDecoderId_MMAL;
    else if (d->hwaccel == "qsv")
        return VideoDecoderId_QSV;
    else if (d->hwaccel == "crystalhd")
        return VideoDecoderId_CrystalHD;
    return VideoDecoderId_FFmpeg;
}

std::string VideoDecoderFFmpeg::description() const
{
    const int patch = SMI_VERSION_PATCH((int)avcodec_version());
    return "";
//    return Util::sformat("%s avcodec %d->%d->%d",
//            (patch >= 100 ? "FFmpeg" : "Libav"),
//            (SMI_VERSION_MAJOR((int)avcodec_version())),
//            (SMI_VERSION_MINOR((int)avcodec_version())),
//            patch);
}

void VideoDecoderFFmpeg::setSkipLoopFilter(VideoDecoderFFmpeg::DiscardType value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->skip_loop_filter = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "skip_loop_filter", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipLoopFilter() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (DiscardType)d->skip_loop_filter;
}

void VideoDecoderFFmpeg::setSkipIDCT(VideoDecoderFFmpeg::DiscardType value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->skip_idct = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "skip_idct", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipIDCT() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (DiscardType)d->skip_idct;
}

void VideoDecoderFFmpeg::setStrict(VideoDecoderFFmpeg::StrictType value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->strict = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "strict", (int64_t)value, 0);
}

VideoDecoderFFmpeg::StrictType VideoDecoderFFmpeg::strict() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (StrictType)d->strict;
}

void VideoDecoderFFmpeg::setSkipFrame(VideoDecoderFFmpeg::DiscardType value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->skip_frame = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "skip_frame", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipFrame() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (DiscardType)d->skip_frame;
}

void VideoDecoderFFmpeg::setThreads(int value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->threads = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "threads", (int64_t)value, 0);
}

int VideoDecoderFFmpeg::threads() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return d->threads;
}

void VideoDecoderFFmpeg::setThreadFlags(VideoDecoderFFmpeg::ThreadFlag value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->thread_type = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "thread_type", (int64_t)value, 0);
}

VideoDecoderFFmpeg::ThreadFlag VideoDecoderFFmpeg::threadFlags() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (ThreadFlag)d->thread_type;
}

void VideoDecoderFFmpeg::setMotionVectorVisFlags(VideoDecoderFFmpeg::MotionVectorVisFlag value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->vismv = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "vismv", (int64_t)value, 0);
}

VideoDecoderFFmpeg::MotionVectorVisFlag VideoDecoderFFmpeg::motionVectorVisFlags() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (MotionVectorVisFlag)d->vismv;
}

void VideoDecoderFFmpeg::setBugFlags(VideoDecoderFFmpeg::BugFlag value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->bug = value;
    if (d->codec_ctx)
        av_opt_set_int(d->codec_ctx, "bug", (int64_t)value, 0);
}

VideoDecoderFFmpeg::BugFlag VideoDecoderFFmpeg::bugFlags() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return (BugFlag)d->bug;
}

void VideoDecoderFFmpeg::setHwaccel(const std::string &value) {
    DPTR_D(VideoDecoderFFmpeg);
    d->hwaccel = value;
}

std::string VideoDecoderFFmpeg::hwaccel() const {
    DPTR_D(const VideoDecoderFFmpeg);
    return d->hwaccel;
}

NAMESPACE_END
