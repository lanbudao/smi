#ifndef AVPLAYER_P_H
#define AVPLAYER_P_H

#include "sdk/player.h"
#include "Demuxer.h"
#include "AVDemuxThread.h"
#include "VideoThread.h"
#include "AudioThread.h"
#include "PacketQueue.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "AudioOutput.h"
#include "OutputSet.h"
#include "AVLog.h"
#include "AVClock.h"
#include "VideoRenderer.h"
#include "subtitle/Subtitle.h"
#include "filter/Filter.h"

NAMESPACE_BEGIN

class PlayerPrivate
{
public:
    PlayerPrivate() :
        loaded(false),
        paused(false),
        seeking(false),
        buffer_mode(BufferPackets),
        buffer_value(-1),
        demuxer(nullptr),
        demux_thread(nullptr),
        video_thread(nullptr),
        audio_thread(nullptr),
        video_dec(nullptr),
        audio_dec(nullptr),
        ao(nullptr),
        resample_type(ResampleBase),
        clock_type(SyncToAudio)
    {
        ao = new AudioOutput;
        demuxer = new Demuxer();
        demuxer->setMediaInfo(&mediainfo);
        demux_thread = new AVDemuxThread();
        demux_thread->setDemuxer(demuxer);
        demux_thread->setClock(&clock);
        video_dec_ids = VideoDecoder::registered();
    }

    ~PlayerPrivate()
    {
        if (ao) {
            if (ao->isOpen())
                ao->close();
            delete ao;
            ao = nullptr;
        }
        if (audio_dec) {
            delete audio_dec;
            audio_dec = nullptr;
        }
        if (video_dec) {
            delete video_dec;
            video_dec = nullptr;
        }
        if (demuxer) {
            delete demuxer;
            demuxer = nullptr;
        }
        if (demux_thread) {
            demux_thread->stop();
            delete demux_thread;
            demux_thread = nullptr;
        }
//        audio_output_set.clearOutput();
        video_output_set.clearOutput();
    }

	void initRenderVideo();

    void playInternal();

	bool setupAudioThread();
	bool setupVideoThread();

	void updateBufferValue(PacketQueue* buf);

    bool installFilter(AudioFilter* f, int index);

    bool installFilter(VideoFilter* f, VideoRenderer* render, int index);

    bool installFilter(RenderFilter* f, VideoRenderer* render, int index);

    bool insertFilter(std::list<Filter*> &filters, Filter* f, int index);

    bool applySubtitleStream();

    void onSeekFinished(void*) { seeking = false; }

    std::string url;
    bool loaded;
    bool paused;
    bool seeking;
    std::hash<std::string> format_dict;
	uint8_t buffer_mode;
	int64_t buffer_value;

    /*Demuxer*/
    Demuxer *demuxer;
    AVDemuxThread *demux_thread;

    /*AVThread*/
    AVThread *video_thread;
    AVThread *audio_thread;

//    int video_track, audio_track, sub_track;

    /*Decoder*/
    VideoDecoder *video_dec;
    AudioDecoder *audio_dec;
    std::vector<VideoDecoderId> video_dec_ids;

    /*OutputSet*/
    OutputSet video_output_set;
    OutputSet audio_output_set;
    AudioOutput *ao;

    AVClock clock;
    ClockType clock_type;
    ResampleType resample_type;

    /*Subtitles*/
    Subtitle internal_subtitle;
    std::list<Subtitle*> external_subtitles;

    /* filters*/
    std::list<Filter*> audio_filters;
    std::list<Filter*> video_filters;
    std::map<VideoRenderer*, std::list<Filter*>> renderToFilters;
    std::list<Filter*> render_filters;

    MediaInfo mediainfo;

    std::function<void(MediaStatus s)> mediaStatusChanged;
    std::function<void(MediaType type, int stream)> mediaStreamChanged;
    std::function<void(MediaInfo*)> subtitleHeaderChanged;
    std::function<void(double pos, double incr, SeekType type)> seekRequest;
};

void PlayerPrivate::playInternal()
{
    if (!setupAudioThread()) {
        demux_thread->setAudioThread(nullptr);
        if (audio_thread) {
            delete audio_thread;
            audio_thread = nullptr;
        }
    }
    if (!setupVideoThread()) {
        demux_thread->setVideoThread(nullptr);
        if (video_thread) {
            delete video_thread;
            video_thread = nullptr;
        }
    }

    /*Set Clock Type*/
    if (clock_type == SyncToAudio) {
        if (audio_thread) {
            clock_type = SyncToAudio;
        } else {
            clock_type = SyncToExternalClock;
        }
    } else if (clock_type == SyncToVideo) {
        if (video_thread) {
            clock_type = SyncToVideo;
        } else {
            clock_type = SyncToAudio;
        }
    }
	clock.init(SyncToExternalClock, clock.serialAddr(SyncToExternalClock));
    clock.setClockType(clock_type);
	clock.setEof(false);
    demux_thread->start();
}

void PlayerPrivate::initRenderVideo()
{
	for (AVOutput* output : video_output_set.outputs()) {
		VideoRenderer* render = static_cast<VideoRenderer*>(output);
		render->initVideoRender();
	}
}

bool PlayerPrivate::setupAudioThread()
{
    // No audio track
    if (!mediainfo.audio)
        return false;
	if (audio_thread) {
		audio_thread->packets()->clear();
		audio_thread->setDecoder(nullptr);
	}
	if (audio_dec) {
		delete audio_dec;
		audio_dec = nullptr;
    }
    if (!demuxer->stream(MediaTypeAudio))
        return false;

	AudioDecoder *dec = AudioDecoder::create();
	if (!dec)
        return false;
    dec->initialize(demuxer->formatCtx(), demuxer->stream(MediaTypeAudio));
	if (!dec->open()) {
		return false;
	}
	audio_dec = dec;
	if (!audio_dec) {
		AVDebug("Can not found audio decoder.\n");
		return false;
	}

    AudioFormat af;
    af.setSampleRate(mediainfo.audio->sample_rate);
    af.setChannelLayoutFFmpeg(mediainfo.audio->channel_layout);
    af.setSampleFormatFFmpeg(mediainfo.audio->format);
    af.setChannels(mediainfo.audio->channels);
	if (!af.isValid()) {
		AVWarning("Invalid audio format, disable audio!");
		return false;
	}

    ao->setResampleType(resample_type);
	ao->setAudioFormat(af);
	ao->close();
	if (!ao->open()) {
		return false;
	}

	if (!audio_thread) {
		audio_thread = new AudioThread();
		audio_output_set.addOutput(ao);
        audio_thread->setMediaInfo(&mediainfo);
		audio_thread->setOutputSet(&audio_output_set);
		audio_thread->setClock(&clock);
        audio_thread->updateFilters(audio_filters);
        clock.init(SyncToAudio, audio_thread->packets()->serialAddr());
		demux_thread->setAudioThread(audio_thread);
	}    
    audio_dec->setResampleType(resample_type);
	audio_thread->setDecoder(audio_dec);
	updateBufferValue(audio_thread->packets());
	return true;
}

bool PlayerPrivate::setupVideoThread()
{
    if (!mediainfo.video)
        return false;   // No video track
	if (video_thread) {
		video_thread->packets()->clear();
		video_thread->setDecoder(nullptr);
	}

	if (video_dec) {
		delete video_dec;
		video_dec = nullptr;
	}
    if (!demuxer->stream(MediaTypeVideo))
        return false;

	for (size_t i = 0; i < video_dec_ids.size(); ++i) {
		VideoDecoder *dec = VideoDecoder::create(video_dec_ids.at(i));
		if (!dec)
			continue;
        dec->initialize(demuxer->formatCtx(), demuxer->stream(MediaTypeVideo));
		if (dec->open()) {
			video_dec = dec;
			break;
		}
		delete dec;
	}
	if (!video_dec) {
		AVDebug("Can not found video decoder.\n");
		return false;
	}
	if (!video_thread) {
        video_thread = new VideoThread();
        video_thread->setMediaInfo(&mediainfo);
		video_thread->setOutputSet(&video_output_set);
		video_thread->setClock(&clock);
        clock.init(SyncToVideo, video_thread->packets()->serialAddr());
		demux_thread->setVideoThread(video_thread);
	}
	video_thread->setDecoder(video_dec);
	updateBufferValue(video_thread->packets());
	return true;
}

// TODO: set to a lower value when buffering
void PlayerPrivate::updateBufferValue(PacketQueue* buf)
{
	const bool is_video = video_thread && buf == video_thread->packets();
	const double fps = std::max<double>(24.0, mediainfo.video->frame_rate.toDouble());
	int64_t bv = 24;
	if (buffer_mode == BufferTime)
		bv = 1000LL; //ms
	else if (buffer_mode == BufferBytes)
		bv = 1024LL;
	// do not block video packet if is music with cover
	int64_t frames = mediainfo.video->frames;
	if (is_video && mediainfo.video) {
		if (demuxer->hasAttachedPic() || (frames > 0 && frames < bv))
			bv = 0;
	}
    buf->setRealTime(demuxer->isRealTime());
	buf->setBufferMode((BufferMode)buffer_mode);
	buf->setBufferValue(buffer_value < 0LL ? bv : buffer_value);
}

inline bool PlayerPrivate::installFilter(AudioFilter * filter, int index)
{
    if (!audio_thread)
        return false;
    if (!insertFilter(audio_filters, filter, index))
        return false;
    audio_thread->updateFilters(audio_filters);
    return true;
}

inline bool PlayerPrivate::installFilter(VideoFilter * filter, VideoRenderer * render, int index)
{
    if (render) { // install to videorender appointed
        if (!insertFilter(renderToFilters[render], filter, index))
            return false;
        render->updateFilters(renderToFilters[render]);
    }
    else { // install to every videorender
        if (!video_thread)
            return false;
        if (!insertFilter(video_filters, filter, index))
            return false;
        video_thread->updateFilters(video_filters);
    }
    return true;
}

inline bool PlayerPrivate::installFilter(RenderFilter * filter, VideoRenderer * render, int index)
{
    if (render) { // install to videorender appointed
        if (!insertFilter(renderToFilters[render], filter, index))
            return false;
        render->updateFilters(renderToFilters[render]);
    }
    else { // install to every videorender
        std::map<VideoRenderer*, std::list<Filter*>>::iterator it = renderToFilters.begin();
        for (; it != renderToFilters.end(); ++it) {
            VideoRenderer *r = (VideoRenderer *)(it->first);
            if (!insertFilter(it->second, filter, index))
                return false;
            r->updateFilters(it->second);
        }
    }
    return true;
}

inline bool PlayerPrivate::insertFilter(std::list<Filter*>& filters, Filter * f, int index)
{
    int p = index;
    if (p < 0)
        p += filters.size();
    if (p < 0)
        p = 0;
    if (p > filters.size())
        p = filters.size();
    std::list<Filter*>::iterator it_dst2;
    std::list<Filter*>::iterator it_dst = filters.begin();
    while (--p >= 0) {
        it_dst++;
    }
    std::list<Filter*>::iterator it_src = std::find(filters.begin(), filters.end(), f);
    if ((it_dst == it_src) && !filters.empty())
        return true;
    filters.remove(f);
    filters.insert(it_dst, f);
    return true;
}

bool PlayerPrivate::applySubtitleStream()
{
    if (!mediainfo.subtitle)
        return false;
    if (subtitleHeaderChanged) {
        subtitleHeaderChanged(&mediainfo);
    }
    return true;
}

NAMESPACE_END
#endif //AVPLAYER_P_H
