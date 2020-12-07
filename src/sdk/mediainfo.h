#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <vector>
#include <string>

typedef struct Rational {
    int num; ///< Numerator
    int den; ///< Denominator
    Rational(int n = 0, int d = 0) { num = n; den = d; }
    double toDouble() { return num / (double) den; }
} Rational;

typedef struct AudioStreamInfo_ {
    int stream;
    int64_t start_time; /* ms */
    int64_t duration; /* ms */
    int64_t frames;

    /* Codec Paras */
    std::string codec_name;
    int sample_rate;
    int profile;
    int level;
    int format;
    std::string sample_format_s;  /* the sample format, the value corresponds to enum AVSampleFormat. */
    int channels;
    int frame_size;
    uint64_t channel_layout;
    std::string channel_layout_s;
    Rational time_base;
} AudioStreamInfo;

typedef struct VideoStreamInfo_ {
    int stream;
    int64_t start_time; /* ms */
    int64_t duration; /* ms */
    int64_t frames;

    /* Codec Paras */
    std::string codec_name, codec_long;
    int width, height;
    int codec_width, codec_height;
    float rotate = 0.0;
    double pts;
    Rational display_aspect_ratio;
    int pixel_format;   /* the pixel format, the value corresponds to enum AVPixelFormat. */
    int delay;
    Rational frame_rate;
    Rational sample_aspect_ratio; // video only
    Rational time_base;
} VideoStreamInfo;

typedef struct SubtitleStreamInfo_ {
    int stream;
    int64_t start_time; /* ms */
    int64_t duration; /* ms */
    int64_t frames;

    /* Codec Paras */
    std::string codec_name, codec_long;
    uint8_t *extradata;
    int extradata_size;
    Rational time_base;
} SubtitleStreamInfo;

typedef struct MediaInfo_ {
    std::string url;
    int64_t start_time;
    int64_t duration;
    int64_t bit_rate;
    int64_t size;
    int streams;

    std::vector<AudioStreamInfo> audios;
    std::vector<VideoStreamInfo> videos;
    std::vector<SubtitleStreamInfo> subtitles;

    AudioStreamInfo* audio = nullptr;
    VideoStreamInfo* video = nullptr;
    SubtitleStreamInfo* subtitle = nullptr;

} MediaInfo;

#endif // MEDIAINFO_H
