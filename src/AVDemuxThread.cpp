#include "AVDemuxThread.h"
#include "Demuxer.h"
#include "AudioThread.h"
#include "VideoThread.h"
#include "PacketQueue.h"
#include "AVLog.h"
#include "AVClock.h"
#include <mutex>
extern "C" {
#include "libavformat/avformat.h"
}

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

NAMESPACE_BEGIN

class AVDemuxThreadPrivate
{
public:
    AVDemuxThreadPrivate():
        demuxer(nullptr),
        audio_thread(nullptr),
        video_thread(nullptr),
        stopped(true),
        paused(false),
        last_paused(false),
        main_buffer(nullptr),
        buffering(false),
        lastProgress(0),
        seek_req(false),
        clock(nullptr),
        eof(false)
    {

    }
    ~AVDemuxThreadPrivate()
    {

    }

    //bool packetsEnough(AVStream* s, PacketQueue* queue)
    //{
    //    return !s ||
    //           (s->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
    //           (queue->size() > MIN_FRAMES && (queue->duration() > 1.0));
    //}

    //bool packetsFull()
    //{
    //     bool enough = packetsEnough(demuxer->stream(MediaTypeAudio), audio_thread->packets()) &&
    //        packetsEnough(demuxer->stream(MediaTypeVideo), audio_thread->packets());
    //     //TODO: Should we calculate the storage size of packets?
    //     return enough;
    //}

    Demuxer *demuxer;
    AVThread *audio_thread, *video_thread;
    bool stopped;
    bool paused, last_paused;
    PacketQueue* main_buffer;
    bool buffering;
    float lastProgress;

    bool seek_req; 
	double seek_pos, seek_incr;
	SeekType seek_type;
    AVClock *clock;
    bool eof;   /*Enqueue a eof packet if demuxer is at end*/
    std::mutex wait_mutex;
    std::condition_variable continue_read_cond;

    /* callback */
    std::function<void(float p)> bufferProcessChanged;
    std::function<void(MediaStatus s)> mediaStatusChanged;
    std::function<void(Packet*pkt)> subtitlePacketChanged;
};

AVDemuxThread::AVDemuxThread():
    CThread("demux"),
    d_ptr(new AVDemuxThreadPrivate)
{

}

AVDemuxThread::~AVDemuxThread() {

}

void AVDemuxThread::stop()
{
    DPTR_D(AVDemuxThread);
	if (d->stopped)
        return;
    d->stopped = true;
    this->wait();
    //if (d->audio_thread) {
    //    d->audio_thread->packets()->clear();
    //    d->audio_thread->packets()->blockFull(false);
    //    d->audio_thread->stop();
    //    d->audio_thread->wait();
    //    delete d->audio_thread;
    //    d->audio_thread = nullptr;
    //}
    //if (d->video_thread) {
    //    d->video_thread->packets()->clear();
    //    d->video_thread->packets()->blockFull(false);
    //    d->video_thread->stop();
    //    d->video_thread->wait();
    //    delete d->video_thread;
    //    d->video_thread = nullptr;
    //}
}

void AVDemuxThread::pause(bool p)
{
    DPTR_D(AVDemuxThread);
    if (d->buffering) {
        AVDebug("can not to pause when packets is buffering\n");
        return;
    }
	d->paused = p;
    d->last_paused = p;
	if (d->audio_thread)
		d->audio_thread->pause(p);
	if (d->video_thread)
		d->video_thread->pause(p);
}

void AVDemuxThread::seek(double pos, double incr, SeekType type)
{
    DPTR_D(AVDemuxThread);

    /*TODO: the real-time media can do seek?*/
    if (d->buffering) {
        AVDebug("can not to seek when packets is buffering\n");
        return;
    }
	if (!d->seek_req) {
		///* if pos is nan, it indicates that the previous seek operation has not been completed */
		//if (isnan(pos))
		//	return;
		//if (d->clock->type() != SyncToVideo && isnan(d->clock->value(SyncToVideo))) {
		//	AVDebug("Video seek is not finished.\n");
		//	return;
		//}
		d->seek_req = true;
		d->seek_pos = pos;
		d->seek_incr = incr;
		d->seek_type = type;
		d->continue_read_cond.notify_one();
	}
}

void AVDemuxThread::setDemuxer(Demuxer *demuxer)
{
    DPTR_D(AVDemuxThread);
    d->demuxer = demuxer;
}

void AVDemuxThread::setAudioThread(AVThread *thread)
{
    DPTR_D(AVDemuxThread);
    d->audio_thread = thread;
}

AVThread *AVDemuxThread::audioThread()
{
    DPTR_D(const AVDemuxThread);
    return d->audio_thread;
}

void AVDemuxThread::setVideoThread(AVThread *thread)
{
    DPTR_D(AVDemuxThread);
    d->video_thread = thread;
}

AVThread *AVDemuxThread::videoThread()
{
    DPTR_D(const AVDemuxThread);
    return d->video_thread;
}

void AVDemuxThread::setClock(AVClock *clock)
{
    d_func()->clock = clock;
}

void AVDemuxThread::stepToNextFrame()
{
	DPTR_D(AVDemuxThread);
	pause(false);
	if (d->video_thread) {
		VideoThread* thread = dynamic_cast<VideoThread*>(d->video_thread);
		thread->stepToNextFrame([this]() {
			this->pause(true);
		});
	}
}

void AVDemuxThread::updateBufferStatus()
{
    DPTR_D(AVDemuxThread);
    if (!d->main_buffer || !d->demuxer->isRealTime())
        return;
    if (d->buffering && d->bufferProcessChanged) {
        float progress = d->main_buffer->bufferProgress();
        if (progress - d->lastProgress > 0.01) {
            d->bufferProcessChanged(progress);
            d->lastProgress = progress;
        }
    }
    if (d->buffering == d->main_buffer->isBuffering())
        return;
    d->buffering = d->main_buffer->isBuffering();
    d->lastProgress = 0;
    if (d->mediaStatusChanged) {
        d->mediaStatusChanged(d->buffering ? Buffering : Buffered);
    }
}

void AVDemuxThread::setMediaStatusChangedCB(std::function<void(MediaStatus s)> f)
{
    d_func()->mediaStatusChanged = f;
}

void AVDemuxThread::setBufferProcessChangedCB(std::function<void(float p)> f)
{
    d_func()->bufferProcessChanged = f;
}

void AVDemuxThread::setSubtitlePacketCallback(std::function<void(Packet*pkt)> f)
{
    d_func()->subtitlePacketChanged = f;
}

void AVDemuxThread::run()
{
    DPTR_D(AVDemuxThread);
    int stream;
    Packet pkt;
    int ret = -1;

    PacketQueue *vbuffer = d->video_thread ? d->video_thread->packets() : nullptr;
    PacketQueue *abuffer = d->audio_thread ? d->audio_thread->packets() : nullptr;
    Demuxer *demuxer = d->demuxer;
    AVThread* thread = !d->video_thread || (d->audio_thread && demuxer->hasAttachedPic())
        ? d->audio_thread : d->video_thread;
    d->main_buffer = thread->packets();
	if (abuffer) {
        abuffer->enqueue(Packet::createFlush());
        //Why set it false before...
		//abuffer->blockEmpty(false);
    }
	if (vbuffer) {
		vbuffer->enqueue(Packet::createFlush());
		//vbuffer->blockEmpty(false);
    }
    if (d->audio_thread && !d->audio_thread->isRunning()) {
        d->audio_thread->start();
    }
    if (d->video_thread && !d->video_thread->isRunning()) {
        d->video_thread->start();
    }
    bool audio_has_pic = false;
    d->stopped = false;

    while (true) {
        if (d->stopped)
            break;
        //if (d->paused) {
        //    continue;
        //}
        if (d->seek_req) {
            d->demuxer->setSeekType(d->seek_type);
            if (d->demuxer->seek(d->seek_pos, d->seek_incr)) {
                if (abuffer) {
                    abuffer->clear();
                    abuffer->enqueue(Packet::createFlush());
                    d->audio_thread->requestSeek();
                }
                if (vbuffer) {
                    vbuffer->clear();
                    vbuffer->enqueue(Packet::createFlush());
                    d->video_thread->requestSeek();
                }
            }
            d->seek_req = false;
            d->eof = false;
			d->clock->setEof(false);
            if (d->paused) {
				stepToNextFrame();
            }
        }
        /* pause if is buffering*/
        if (d->demuxer->isRealTime()) {
            if (d->buffering) {
                if (!d->paused) {
                    if (d->audio_thread)
                        d->audio_thread->pause(true);
                    if (d->video_thread)
                        d->video_thread->pause(true);
                }
            } else {
                if (d->audio_thread)
                    d->audio_thread->pause(d->last_paused);
                if (d->video_thread)
                    d->video_thread->pause(d->last_paused);
            }
        }
        audio_has_pic = demuxer->hasAttachedPic();
		if ((!abuffer || (abuffer && abuffer->checkFull())/* || (abuffer->duration() > 1.0)*/) &&
			(!(vbuffer && !audio_has_pic) || (vbuffer && vbuffer->checkFull())/* || (vbuffer->duration() > 1.0)*/)) {
			/* wait 10 ms */
			std::unique_lock<std::mutex> lock(d->wait_mutex);
			d->continue_read_cond.wait_for(lock, std::chrono::milliseconds(10));
			continue;
		}
        ret = demuxer->readFrame();
        if (ret < 0) {
            if (ret == AVERROR_EOF && !d->eof) {
                if (abuffer) {
                    abuffer->enqueue(Packet::createEOF());
                }
                if (vbuffer) {
                    vbuffer->enqueue(Packet::createEOF());
                }
				d->eof = true;
				d->clock->setEof(true);
            }
            std::unique_lock<std::mutex> lock(d->wait_mutex);
            d->continue_read_cond.wait_for(lock, std::chrono::milliseconds(10));
			continue;
        } else {
			d->eof = false;
			d->clock->setEof(false);
        }
        stream = demuxer->stream();
        pkt = demuxer->packet();

        if (stream == demuxer->streamIndex(MediaTypeVideo)) {
            if (vbuffer) {
				vbuffer->blockFull(false);
                vbuffer->enqueue(pkt);
            }
        }
        else if (stream == demuxer->streamIndex(MediaTypeAudio)) {
            if (abuffer) {
				abuffer->blockFull(false);
                abuffer->enqueue(pkt);
            }
        }
        else if (stream == demuxer->streamIndex(MediaTypeSubtitle)) {
            if (d->subtitlePacketChanged)
                d->subtitlePacketChanged(&pkt);
        }
        this->updateBufferStatus();
    }
    d->stopped = true;
    CThread::run();
}

NAMESPACE_END
