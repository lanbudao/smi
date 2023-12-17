#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <list>
#include <vector>
#include <string>

typedef struct Chapter_ {
    int index;
    int begin;
    std::string begin_str; //hh:mm:ss
    int end;
    std::string end_str; //hh:mm:ss
} Chapter;

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

    std::list<AudioStreamInfo> audios;
    std::list<VideoStreamInfo> videos;
    std::list<SubtitleStreamInfo> subtitles;

    AudioStreamInfo *audio = nullptr;
    VideoStreamInfo *video = nullptr;
    SubtitleStreamInfo *subtitle = nullptr;

    /* Current index of stream selected, 0 by default and -1 means no stream*/
    int audio_track = -1;
    int video_track = -1;
    int subtitle_track = -1;

} MediaInfo;

/**
 *\brief DVD information
 */
typedef struct DVDTitleInfo_ {
    int nr_of_chapters;
    std::vector<Chapter> chapters;
    VideoStreamInfo video;                  /* only one video stream */
    int nr_of_audios;
    std::list<AudioStreamInfo> audios;      /* 9 audio streams at most */
    int nr_of_subtitles;
    std::list<SubtitleStreamInfo> subtitles;/* 23 subtitle streams at most */

    /* Current audio and subtitle stream*/
    AudioStreamInfo *audio = nullptr;
    SubtitleStreamInfo *subtitle = nullptr;
} DVDTitleInfo;

typedef struct DVDInfo_ {
    /* Video Manager Information Management Table */
    std::string vmg_identifier;
    uint8_t  specification_version;
    uint32_t vmg_category;
    uint32_t vmg_region_code;
    uint16_t vmg_nr_of_volumes;   /* VMG Number of Volumes */
    uint16_t vmg_this_volume_nr;  /* VMG This Volume */
    uint16_t vmg_nr_of_title_sets;  /* Number of VTSs. */

    uint32_t first_play_pgc;      /* Start byte of First Play PGC (FP PGC) */
    
    /* video title info */
    std::list<DVDTitleInfo> titles;

} DVDInfo;

#endif // MEDIAINFO_H
