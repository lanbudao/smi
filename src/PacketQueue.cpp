#include "PacketQueue.h"
#include "AVLog.h"
#include "utils/innermath.h"

extern "C" {
#include <libavutil/time.h>
}

NAMESPACE_BEGIN

typedef struct {
    int64_t v; //pts, total packes or total bytes
    int64_t bytes; //total bytes
    int64_t t;
} BufferInfo;

static const int kAvgSize = 16;
class PacketQueuePrivate
{
public:
    PacketQueuePrivate():
        duration(0),
        serial(0),
		mode(BufferPackets),
		buffering(true), // in buffering state at the beginning
		max(1.5),
        realtime(false),
		buffer(24),
		value0(0),
		value1(0)/*,
		history(kAvgSize)*/
    {

    }

	float calc_speed(bool use_bytes) const;

    double duration;
    int serial;
	BufferMode mode;
	bool buffering;
	double max;
    bool realtime;
	// bytes or count
	int64_t buffer;
	int64_t value0, value1;
    std::vector<BufferInfo> record;
};


PacketQueue::PacketQueue():
    d_ptr(new PacketQueuePrivate)
{

}

PacketQueue::~PacketQueue() {

}

double PacketQueue::duration() const
{
    DPTR_D(const PacketQueue);
    return d->duration;
}

int PacketQueue::serial() const
{
    DPTR_D(const PacketQueue);
    return d->serial;
}

int *PacketQueue::serialAddr()
{
    DPTR_D(PacketQueue);
    return &d->serial;
}

bool PacketQueue::prepared() const
{
    DPTR_D(const PacketQueue);
    return !d->realtime || (d->realtime && !d->buffering);
}

void PacketQueue::setRealTime(bool r)
{
    DPTR_D(PacketQueue);
    d->realtime = r;
}

void PacketQueue::setBufferMode(BufferMode m)
{
	DPTR_D(PacketQueue);
	d->mode = m;
	if (checkEmpty()) {
		d->value0 = d->value1 = 0;
		return;
	}
	if (d->mode == BufferTime) {
		d->value0 = int64_t(q.front().pts*1000.0);
	}
	else {
		d->value0 = 0;
	}
}

BufferMode PacketQueue::bufferMode() const
{
	return d_func()->mode;
}

void PacketQueue::setBufferValue(int64_t value)
{
	d_func()->buffer = value;
}

int64_t PacketQueue::bufferValue() const
{
	return d_func()->buffer;
}

void PacketQueue::setBufferMax(double max)
{
	DPTR_D(PacketQueue);
	if (max < 1.0) {
		AVWarning("max (%f) must >= 1.0\n", max);
		return;
	}
	d->max = max;
}

double PacketQueue::bufferMax() const
{
	DPTR_D(const PacketQueue);
	return d->max;
}

int64_t PacketQueue::buffered() const
{
	DPTR_D(const PacketQueue);
	return d->value1 - d->value0;
}

bool PacketQueue::isBuffering() const
{
	return d_func()->buffering;
}

float PacketQueue::bufferProgress() const
{
	const float p = FORCE_FLOAT(buffered() * 1.0 / bufferValue());
	return std::max<float>(std::min<float>(p, 1.0), 0.0);
}

float PacketQueue::bufferSpeed() const
{
	DPTR_D(const PacketQueue);
	return d->calc_speed(false);
}

float PacketQueue::bufferSpeedInBytes() const
{
	DPTR_D(const PacketQueue);
	return d->calc_speed(true);
}

bool PacketQueue::checkEnough() const
{
	return buffered() >= bufferValue();
}

bool PacketQueue::checkFull() const
{
    //int64_t buffer = buffered();
    //int64_t default = int64_t(bufferValue() * bufferMax());
    bool full = buffered() >= int64_t(bufferValue() * bufferMax());
	return full;
}

void PacketQueue::onEnqueue(const Packet & pkt)
{
	DPTR_D(PacketQueue);
	if (pkt.isFlush()) {
		d->serial++;
		d->duration = 0;
	}
	else {
		d->duration += pkt.duration;
	}
	pkt.serial = d->serial;

    if (pkt.isFlush() || pkt.isEOF())
        return;
	if (d->mode == BufferTime) {
		d->value1 = FORCE_INT64(pkt.pts * 1000.0);
        if (d->value1 < 0) {
            /* some real-time stream have not pts in packet. If not, switch buffer mode to BufferBytes*/
            setBufferMode(BufferBytes);
            d->value1 += pkt.size;
            AVDebug("pts is null\n");
        }
        else {
            if (d->value0 == 0)
                d->value0 = FORCE_INT64(pkt.pts * 1000.0);
        }
    }
	else if (d->mode == BufferBytes) {
        d->value1 += pkt.size;
	}
	else {
		d->value1++;
	}
    if (!d->buffering) {
        d->record.clear();
        return;
    }
	if (checkEnough()) {
		d->buffering = false;
	}
	BufferInfo info;
    info.bytes = pkt.size;
	if (!d->record.empty())
        info.bytes += d->record.back().bytes;
    info.v = d->value1;
    info.t = static_cast<int64_t>(av_gettime_relative() / 1000.0);
	d->record.push_back(info);
}

void PacketQueue::onDequeue(const Packet & pkt)
{
	DPTR_D(PacketQueue);
	if (!pkt.isFlush()) {
		d->duration -= pkt.duration;
	}
	if (checkEmpty()) {
		d->buffering = true;
	}
	if (checkEmpty()) {
		d->value0 = 0;
		d->value1 = 0;
		return;
	}
	if (d->mode == BufferTime) {
        if (q.size() > 0 && q.front().type == Packet::Data)
            d->value0 = int64_t(q.front().pts * 1000.0);
    }
	else if (d->mode == BufferBytes) {
		d->value1 -= pkt.size;
		d->value1 = std::max<int64_t>(0LL, d->value1);
	}
	else {
		d->value1--;
	}
}

float PacketQueuePrivate::calc_speed(bool use_bytes) const
{
    if (record.empty())
        return 0;
    const double dt = (double)av_gettime_relative() / 1000000.0 - record.front().t / 1000.0;
    // dt should be always > 0 because history stores absolute time
    if (FuzzyIsNull(dt))
        return 0;
    const int64_t delta = use_bytes ? 
        record.back().bytes - record.front().bytes :
        record.back().v - record.front().v;
    if (delta < 0) {
        AVWarning("PacketBuffer internal error. delta(bytes %d): %lld\n", use_bytes, delta);
        return 0;
    }
    return static_cast<float>(delta / dt);
}

NAMESPACE_END
