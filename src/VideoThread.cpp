#include "VideoThread.h"
#include "AVThread_p.h"
#include "VideoDecoder.h"
#include "AVLog.h"
#include "OutputSet.h"
#include "innermath.h"
#include "framequeue.h"
#include "subtitle/SubtitleDecoder.h"
#include "subtitle/assrender.h"

extern "C" {
#include "libavutil/time.h"
#include "libavutil/bprint.h"
#include "libavutil/log.h"
}

#define REFRESH_RATE 0.01
NAMESPACE_BEGIN

class SubtitleDecoderThread : public CThread
{
public:
    SubtitleDecoderThread() :
        CThread("subtitle decoder"),
        abort(false),
        pkts(nullptr),
        serial(-1)
    {

    }

    void setAssParseContext(AVCodecContext *ctx, int w, int h)
    {
        ass_render.initialize(ctx, w, h);
    }

    void setPackets(PacketQueue * p)
    {
        pkts = p;
    }

    void stop()
    {
        frames.clear();
        abort = true;
    }
    void run()
    {
        bool pkt_valid = true;
        Packet pkt;
        SubtitleFrame frame;
        flush_dec = false;
        while (true) {
            int ret = 0;
            if (abort)
                break;
            if (flush_dec) {
                pkt = Packet::createFlush();
            }
            else {
                if (!pkts->prepared()) {
                    msleep(1);
                    continue;
                }
                pkt = pkts->dequeue(&pkt_valid, 10);
                if (!pkt_valid) {
                    continue;
                }
                flush_dec = pkt.isEOF();
                serial = pkt.serial;
                if (pkt.serial != pkts->serial()) {
                    AVDebug("The subtitle pkt is obsolete.\n");
                    continue;
                }
                if (pkt.isFlush()) {
                    AVDebug("Seek is required, flush subtitle decoder.\n");
                    decoder->flush();
                    /* must clear the frames buffer for seek*/
                    frames.clear();
                    continue;
                }
            }
            // decode
            frame = decoder->decode(&pkt, &ret);
            if (!frame.valid()) {
                if (ret == AVERROR_EOF) {
                    flush_dec = false;
                }
                continue;
            }
            /* 0 = graphics */
            if (frame.data()->format != 0) {
                ass_render.addSubtitleToTrack(frame.data());
            }
            frame.setSerial(serial);
            frames.enqueue(frame);
        }
        CThread::run();
    }

    bool abort;
    PacketQueue *pkts;
    SubtitleFrameQueue frames;
    SubtitleDecoder* decoder;
    OutputSet* output;
    int serial;
    // flush decoder when media is eof
    bool flush_dec;

    ASSAide::ASSRender ass_render;
};

class VideoDecoderThread: public CThread
{
public:
    VideoDecoderThread():
        CThread("video decoder"),
        abort(false),
        pkts(nullptr),
        serial(-1)
    {

    }
    void stop()
    {
        frames.clear();
        abort = true;
    }
    void run()
    {
        bool pkt_valid = true;
        Packet pkt;
        VideoFrame frame;
        flush_dec = false;
        while (true) {
            int ret = 0;
            if (abort)
                break;
            if (flush_dec) {
                pkt = Packet::createFlush();
            }
            else {
                if (!pkts->prepared()) {
                    msleep(1);
                    continue;
                }
                pkt = pkts->dequeue(&pkt_valid, 10);
                if (!pkt_valid) {
                    continue;
                }
                flush_dec = pkt.isEOF();
                serial = pkt.serial;
                if (pkt.serial != pkts->serial()) {
                    AVDebug("The video pkt is obsolete.\n");
                    continue;
                }
                if (pkt.isFlush()) {
                    AVDebug("Seek is required, flush video decoder.\n");
                    decoder->flush();
                    /* must clear the frames buffer for seek*/
                    frames.clear();
                    continue;
                }
            }
            // decode
            ret = decoder->decode(pkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    flush_dec = false;
                }
                continue;
            }
            frame = decoder->frame();
            if (!frame.isValid()) {
                continue;
            }
            frame.setSerial(serial);
            frames.enqueue(frame);
        }
        CThread::run();
    }

    bool abort;
    PacketQueue *pkts;
    VideoFrameQueue frames;
    VideoDecoder* decoder;
    OutputSet* output;
    int serial;
    // flush decoder when media is eof
    bool flush_dec;
};


class VideoThreadPrivate: public AVThreadPrivate
{
public:
	VideoThreadPrivate() :
		decode_thread(nullptr),
		last_frame_serial(-1),
		last_frame_pts(0.0),
		last_frame_duration(0.0),
		frame_timer(0.0),
		step(false),
        subtitle_decode_thread(nullptr),
        subtitle_decoder(nullptr),
        subtitle_packets(nullptr)
    {
        decode_thread = new VideoDecoderThread;
    }
    virtual ~VideoThreadPrivate()
    {
        if (subtitle_decode_thread) {
            subtitle_decode_thread->stop();
            subtitle_decode_thread->wait();
            delete subtitle_decode_thread;
            subtitle_decode_thread = nullptr;
        }
        if (decode_thread) {
            decode_thread->stop();
            decode_thread->wait();
            delete decode_thread;
            decode_thread = nullptr;
        }
    }

    double duration(VideoFrame* frame, double max_duration)
    {
        double duration = frame->timestamp() - last_frame_pts;
        if (isnan(duration) || duration <= 0 || duration > max_duration)
            return frame->duration();
        else
            return duration;
    }

    double compute_target_delay(double delay)
    {
        double sync_threshold, diff = 0;

        /* update delay to follow master synchronisation source */
        if (clock->type() != SyncToVideo) {
            /* if video is slave, we try to correct big delays by
               duplicating or deleting a frame */
            diff = clock->value(SyncToVideo) - clock->value();
            sync_threshold = std::max(AV_SYNC_THRESHOLD_MIN, std::min(AV_SYNC_THRESHOLD_MAX, delay));
            if (!isnan(diff) && std::abs(diff) < clock->maxDuration()) {
                if (diff <= -sync_threshold)
                    delay = std::max(0.0, delay + diff);
                else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                    delay = delay + diff;
                else if (diff >= sync_threshold)
                    delay = 2 * delay;
            }
        }
//        AVDebug("video: delay=%0.3f A-V=%f\n", delay, -diff);
        return delay;
    }

    /* Frames which have been decoded*/
    VideoFrameQueue frames;

    VideoDecoderThread* decode_thread;
    int last_frame_serial;
    double last_frame_pts, last_frame_duration;
    VideoFrame last_display_frame;
    double frame_timer;
	bool step;
	std::function<void()> stepCallback;

    /* for subtitle */
    SubtitleDecoderThread *subtitle_decode_thread;
    SubtitleDecoder *subtitle_decoder;
    PacketQueue *subtitle_packets;
};

VideoThread::VideoThread():
    AVThread("video", new VideoThreadPrivate)
{

}

VideoThread::~VideoThread()
{

}

void VideoThread::setSubtitleDecoder(AVDecoder * decoder)
{
    DPTR_D(VideoThread);
    d->subtitle_decoder = dynamic_cast<SubtitleDecoder *>(decoder);
    if (d->subtitle_decode_thread)
        delete d->subtitle_decode_thread;
    d->subtitle_decode_thread = new SubtitleDecoderThread;
    d->subtitle_decode_thread->setAssParseContext(
        d->subtitle_decoder->codecCtx(),
        d->decoder->codecCtx()->width,
        d->decoder->codecCtx()->height);
}

PacketQueue * VideoThread::subtitlePackets()
{
    return d_func()->subtitle_packets;
}

void VideoThread::setSubtitlePackets(PacketQueue * packets)
{
    DPTR_D(VideoThread);
    d->subtitle_packets = packets;
    d->subtitle_decode_thread->setPackets(packets);
}

void VideoThread::pause(bool p)
{
    DPTR_D(VideoThread);
    d->paused = p;
    if (d->paused) {
        d->frame_timer += av_gettime_relative() / 1000000.0 - d->clock->clock(SyncToVideo)->last_updated;
		d->clock->updateClock(SyncToVideo, SyncToVideo);
    }
	d->clock->updateClock(SyncToExternalClock, SyncToExternalClock);
	d->continue_refresh_cond.notify_all();
}

void VideoThread::stepToNextFrame(std::function<void()> cb)
{
	DPTR_D(VideoThread);
	d->step = true;
	d->stepCallback = cb;
}

void VideoThread::run()
{
    DPTR_D(VideoThread);

    AVDebug("video thread id:%d.\n", id());
    AVClock *clock = d->clock;
    bool valid = false;
    Packet pkt;

    double last_duration,  delay, time;
    double remaining_time = REFRESH_RATE;
    bool dequeue_req = true;

    d->stopped = false;
	d->decode_thread->pkts = &d->packets;
	//d->decode_thread->frames = &d->frames;
    d->decode_thread->decoder = dynamic_cast<VideoDecoder *>(d->decoder);
    d->decode_thread->output = d->output;
	VideoFrameQueue* frames = &d->decode_thread->frames;
    d->decode_thread->start();
	VideoFrame* frame = &d->last_display_frame;

    // subtitle
    SubtitleFrameQueue *subtitle_frames = nullptr;
    SubtitleFrame sub_frame;
    if (d->subtitle_decode_thread) {
        subtitle_frames = &d->subtitle_decode_thread->frames;
        d->subtitle_decode_thread->decoder = d->subtitle_decoder;
        d->subtitle_decode_thread->start();
    }

    while (true) {
        if (d->stopped)
            break;

		if (d->paused) {
			d->waitForRefreshMs(10);
			continue;
		}

		if (dequeue_req) {
			*frame = frames->dequeue(&valid, 10);
			if (!valid) {
				continue;
			}
			d->last_frame_serial = frame->serial();
			d->last_frame_pts = frame->timestamp();
			d->last_frame_duration = frame->duration();
			dequeue_req = false;
		}

		if (frame->serial() != d->packets.serial()) {
			dequeue_req = true;
			continue;
		}

		if (d->last_frame_serial != frame->serial())
			d->frame_timer = av_gettime_relative() / 1000000.0;

        /* compute nominal last_duration equals the pts of next frame minus the pts of current frame */
        last_duration = d->duration(&d->last_display_frame, clock->maxDuration());
        /*set speed, default is 1.0*/
        last_duration /= clock->speed();
        /* compute the time of current frame to display*/
        delay = d->compute_target_delay(last_duration);

        time = av_gettime_relative() / 1000000.0;
		remaining_time = d->frame_timer + delay - time;
		if (remaining_time > 0) {
            if (remaining_time > 1) {
                /*drop frame if remaining time more than 1 second*/
                /*it works when play real-time stream, but is it perfect?*/
                continue;
            }
			d->waitForRefreshMs(FORCE_INT(remaining_time * 1000));
		}
        /* Set frame_time as the start time of current frame, also is the end time of last frame */
        d->frame_timer += delay;
        if (delay > 0 && time - d->frame_timer > AV_SYNC_THRESHOLD_MAX)
            d->frame_timer = time;

        if (!isnan(frame->timestamp())) {
            clock->updateValue(SyncToVideo, frame->timestamp(),
//                               frame->pos(),
                               frame->serial());
			clock->updateClock(SyncToExternalClock, SyncToVideo);
        }
        // process subtitle
        if (d->subtitle_decode_thread && subtitle_frames) {
            while (true) {
                sub_frame = subtitle_frames->front(&valid, 10);
                if (!valid) {
                    break;
                }
                if (sub_frame.serial() != d->subtitle_packets->serial()) {
                    continue;
                }
                // video is late
                if (frame->timestamp() < sub_frame.start)
                    break;
                sub_frame = subtitle_frames->dequeue(&valid, 10);
                if (!valid) {
                    break;
                }
                if (frame->timestamp() >= sub_frame.start && frame->timestamp() <= sub_frame.end) {
                    // send subtitle frame
                    d->output->lock();
                    d->output->sendSubtitleFrame(sub_frame);
                    d->output->unlock();
                    break;
                }
            }
        }
        applyFilters(frame);
		d->output->lock();
		d->output->sendVideoFrame(*frame);
		d->output->unlock();
        dequeue_req = true;
		d->seek_req = false;
		if (d->step && !d->paused && !isnan(clock->value()) && d->stepCallback) {
			d->step = false;
			d->stepCallback();
		}
        //AVDebug("video frame pts: %.3f\n", frame->timestamp());
        //AVDebug("A-V: %.3f, %.3f, %.3f\n",
        //    clock->value(SyncToAudio),
        //    clock->value(SyncToVideo),
        //    clock->value(SyncToAudio) - clock->value(SyncToVideo));
    }
    d->packets.clear();
    CThread::run();
}

void VideoThread::applyFilters(VideoFrame * frame)
{
    DPTR_D(VideoThread);
    if (!d->filters.empty()) {
        for (std::list<Filter*>::iterator it = d->filters.begin();
            it != d->filters.end(); ++it) {
            VideoFilter *af = static_cast<VideoFilter*>(*it);
            if (!af->enabled())
                continue;
            af->apply(d->media_info, frame);
        }
    }
}
NAMESPACE_END
