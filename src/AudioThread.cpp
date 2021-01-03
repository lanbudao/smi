#include "AudioThread.h"
#include "AVThread_p.h"
#include "AudioFrame.h"
#include "AudioDecoder.h"
#include "AudioOutput.h"
#include "OutputSet.h"
#include "AVLog.h"
#include "AVClock.h"
#include "innermath.h"
#include "framequeue.h"
#include "AudioResample.h"
extern "C" {
#include "libavutil/samplefmt.h"
}

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MIN 20
#define SAMPLE_CORRECTION_PERCENT_MAX 20

NAMESPACE_BEGIN

class AudioDecoderThread: public CThread
{
public:
    AudioDecoderThread():
        CThread("audio decoder"),
        stopped(false),
        pkts(nullptr),
        serial(-1)
    {

    }
	~AudioDecoderThread() 
	{
	}
    void stop()
	{
		frames.clear();
        stopped = true;
    }
    void run()
    {
        bool pkt_valid = true;
        Packet pkt;
        AudioFrame frame;

        while (true) {
            if (stopped)
                break;
            if (!pkts->prepared()) {
                msleep(1);
                continue;
            }
            pkt = pkts->dequeue(&pkt_valid, 10);
			if (!pkt_valid) {
				continue;
			}
            serial = pkt.serial;
            if (pkt.serial != pkts->serial()) {
                AVDebug("The audio pkt is obsolete.\n");
                continue;
            }
            if (pkt.isFlush()) {
                AVDebug("Seek is required, flush audio decoder.\n");
                decoder->flush();
				frames.clear();
                continue;
            }
            if (decoder->decode(pkt) < 0) {
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

    bool stopped;
    PacketQueue *pkts;
    AudioFrameQueue frames;
    AVClock *clock;
    AudioDecoder* decoder;
    int serial;
};

const int8_t AUDIO_DIFF_AVG_NB = 20;
class AudioThreadPrivate: public AVThreadPrivate
{
public:
    AudioThreadPrivate() :
        decode_thread(nullptr),
        audio_diff_cum(0),
        audio_diff_avg_coef(0),
        audio_diff_avg_count(0),
        audio_diff_threshold(0)
    {
        decode_thread = new AudioDecoderThread;
    }
    virtual ~AudioThreadPrivate()
    {
        if (decode_thread) {
            decode_thread->stop();
            decode_thread->wait();
            delete decode_thread;
            decode_thread = nullptr;
        }
    }

    void setResampleParas(AudioOutput *ao);

    int getWantedSamples(int samples, int freq);

    /* Frames which have been decoded*/
    AudioFrameQueue frames;

    AudioDecoderThread* decode_thread;

    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    int audio_diff_avg_count;
    double audio_diff_threshold;
};

void AudioThreadPrivate::setResampleParas(AudioOutput *ao)
{
    if (!ao) return;
    const AudioFormat &fmt = ao->audioFormat();
    /* init averaging filter */
    audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    audio_diff_avg_count = 0;
    /* since we do not have a precise anough audio FIFO fullness,
       we correct audio sync only if larger than this threshold */
    double bytes_per_sec = av_samples_get_buffer_size(NULL, 
        fmt.channels(), 
        fmt.sampleRate(), 
        (AVSampleFormat)fmt.sampleFormatFFmpeg(),
        1);
    int s = fmt.bytesPerSecond();
    audio_diff_threshold = (double)(ao->bufferSize()) / fmt.bytesPerSecond();
}

int AudioThreadPrivate::getWantedSamples(int samples, int freq)
{
    int wanted_nb_samples = samples;

    /* if not master, then we try to remove or add samples to correct the clock */
    double diff, avg_diff;
    int min_nb_samples, max_nb_samples;

    if (clock->type() == SyncToAudio)
        return samples;
    diff = clock->diffToMaster(SyncToAudio);

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
        audio_diff_cum = diff + audio_diff_avg_coef * audio_diff_cum;
        if (audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
            /* not enough measures to have a correct estimate */
            audio_diff_avg_count++;
        }
        else {
            /* estimate the A-V difference */
            avg_diff = audio_diff_cum * (1.0 - audio_diff_avg_coef);

            if (fabs(avg_diff) >= audio_diff_threshold) {
                wanted_nb_samples = samples + (int)(diff * freq);
                min_nb_samples = ((samples * (100 - SAMPLE_CORRECTION_PERCENT_MIN) / 100));
                max_nb_samples = ((samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
            }
            AVDebug("diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                diff, avg_diff, wanted_nb_samples - samples,
                clock->value(SyncToAudio), audio_diff_threshold);
        }
    }
    else {
        /* too big difference : may be initial PTS errors, so
           reset A-V filter */
        audio_diff_avg_count = 0;
        audio_diff_cum = 0;
    }
    return wanted_nb_samples;
}

AudioThread::AudioThread():
    AVThread("audio", new AudioThreadPrivate)
{

}

AudioThread::~AudioThread() {

}

void AudioThread::run()
{
    DPTR_D(AudioThread);

    auto *dec = dynamic_cast<AudioDecoder *>(d->decoder);
    AudioOutput *ao = static_cast<AudioOutput*>(d->output->outputs().front());
    AVClock *clock = d->clock;
    Packet pkt;
    d->stopped = false;
    bool pkt_valid = false;
    AudioResample* resample = dec->audioResample();
    int decodedSize = 0, decodedPos = 0;

    d->setResampleParas(ao);
    d->decode_thread->pkts = &d->packets;
    d->decode_thread->clock = d->clock;
    d->decode_thread->decoder = dynamic_cast<AudioDecoder *>(d->decoder);
    d->decode_thread->start();

    while (true) {
        bool has_ao = ao && ao->isAvailable();
        if (d->stopped)
            break;
        if (d->paused) {
			d->waitForRefreshMs(10);
            continue;
        }
        AudioFrame frame = d->decode_thread->frames.dequeue(&pkt_valid, 10);
        if (!pkt_valid)
			continue;
		if (frame.serial() != d->packets.serial()) {
			continue;
		}
        if (has_ao) {
            applyFilters(&frame);
            int wanted_samples = d->getWantedSamples(frame.samplePerChannel(), frame.format().sampleRate());
            resample->setWantedSamples(wanted_samples);
            if (resample->speed() != clock->speed()) {
                resample->setSpeed(clock->speed());
//                resample->prepare();
            }
            frame.setAudioResampler(resample);
            frame = frame.to(ao->audioFormat());
        }
        if (!frame.isValid())
            continue;
        //Write data to audio device
        const ByteArray &decoded = frame.data();
        decodedSize = decoded.size();
        decodedPos = 0;
        const double byte_rate = frame.format().bytesPerSecond();
        double pts = frame.timestamp();
        clock->updateValue(SyncToAudio, pts, frame.serial());
        //AVDebug("audio frame pts: %.3f\n", frame.timestamp());
        while (decodedSize > 0) {
            if (d->stopped) {
                AVDebug("audio thread stopped after decode\n");
                break;
            }
            if (d->seek_req) {
                d->seek_req = false;
                AVDebug("audio seek during frame writing\n");
                break;
            }
            /* let pause faster */
            if (d->paused) {
                d->waitForRefreshMs(10);
                continue;
            }
            // Write buffersize at most
            const int chunk = std::min(decodedSize, ao->bufferSize());
            const double chunk_delay = (double)chunk / byte_rate;
            if (has_ao && ao->isOpen()) {
                char *decodedChunk = (char *)malloc(chunk);
                memcpy(decodedChunk, decoded.constData() + decodedPos, chunk);
                //Debug("ao.timestamp: %.3f, pts: %.3f, pktpts: %.3f", ao->timestamp(), pts, pkt.pts);
                d->output->lock();
                pts += chunk_delay;
				ao->write(decodedChunk, chunk, pts);
				clock->updateValue(SyncToAudio, pts, frame.serial());
                clock->updateClock(SyncToExternalClock, SyncToAudio);
                //AVDebug("audio frame pts: %.3f\n", pts);
                d->output->unlock();
//                if (!is_external_clock && ao->timestamp() > 0) {//TODO: clear ao buffer
//                    d.clock->updateValue(ao->timestamp());
//                }
                free(decodedChunk);
            } else {
//                d.clock->updateDelay(delay += chunk_delay);
                /*
                 * why need this even if we add delay? and usleep sounds weird
                 * the advantage is if no audio device, the play speed is ok too
                 * So is portaudio blocking the thread when playing?
                 */
                //TODO: avoid acummulative error. External clock?
                //msleep((unsigned long)(chunk_delay * 1000.0));
				d->waitForRefreshMs((unsigned long)(chunk_delay * 1000.0));
            }
            decodedPos += chunk;
            decodedSize -= chunk;
            pts += chunk_delay;
            pkt.pts += chunk_delay; // packet not fully decoded, use new pts in the next decoding
            pkt.dts += chunk_delay;
        }
		//int a = 0;
    }
    d->packets.clear();
    CThread::run();
}

void AudioThread::applyFilters(AudioFrame * frame)
{
    DPTR_D(AudioThread);
    if (!d->filters.empty()) {
        //sort filters by format. vo->defaultFormat() is the last
        for (std::list<Filter*>::iterator it = d->filters.begin();
            it != d->filters.end(); ++it) {
            AudioFilter *af = static_cast<AudioFilter*>(*it);
            if (!af->enabled())
                continue;
            af->apply(nullptr, frame);
        }
    }
}

NAMESPACE_END

