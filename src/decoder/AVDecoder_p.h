#ifndef AVDECODER_P_H
#define AVDECODER_P_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "VideoFrame.h"
#include "resample/AudioResample.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/pixdesc.h"
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN

class AVDecoderPrivate
{
public:
    AVDecoderPrivate():
        codec_ctx(nullptr),
        dict(nullptr),
        opened(false),
        undecoded_size(0),
        frame(nullptr),
        fmt_ctx(nullptr),
//        stream_index(-1),
        decoder_reorder_pts(false),
        current_stream(nullptr),
        start_pts(AV_NOPTS_VALUE)
    {
        avcodec_register_all();
        codec_ctx = avcodec_alloc_context3(nullptr);
        frame = av_frame_alloc();
    }
    virtual ~AVDecoderPrivate()
    {
        /* delete in Frame class*/
//        if (frame) {
//            av_frame_free(&frame);
//            frame = nullptr;
//        }
        if (dict)
            av_dict_free(&dict);
        avcodec_free_context(&codec_ctx);
    }

    /*Internal Open/Close*/
    virtual bool open() {return true;}
    virtual bool close() {return true;}

    void flush();

    bool opened;
    AVCodecContext *codec_ctx;
    std::string codec_name;
    std::string hwaccel;
    std::hash<std::string> options;
    AVDictionary *dict;
    int undecoded_size;
    AVFrame *frame;
    AVFormatContext *fmt_ctx;
//    int stream_index;
    bool decoder_reorder_pts;
    AVStream* current_stream;

    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    StringList hardware_supports;
};

class AudioDecoderPrivate: public AVDecoderPrivate
{
public:
    AudioDecoderPrivate():
        resample(nullptr),
        resample_type(ResampleBase)
    {

    }
    virtual ~AudioDecoderPrivate()
    {
        if (resample) {
            delete resample;
            resample = nullptr;
        }
    }

    AudioResample *resample;
    ResampleType resample_type;
};

class SubtitleDecoderPrivate: public AVDecoderPrivate
{
public:
    SubtitleDecoderPrivate()
    {

    }
    virtual ~SubtitleDecoderPrivate()
    {

    }
};

NAMESPACE_END
#endif //AVDECODER_P_H
