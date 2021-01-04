#include "AVClock.h"
#include "AVLog.h"
#include "math.h"
#include "sdk/global.h"
extern "C" {
#include "libavutil/time.h"
#include "libavutil/common.h"
}

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

NAMESPACE_BEGIN

class AVClockPrivate
{
public:
    AVClockPrivate():
        pts(0.0),
        pos(0),
        clock_type(SyncToAudio),
        max_frame_duration(0.0),
        paused(false),
        speed(1.0f),
		eof(true)
    {

    }
    ~AVClockPrivate()
    {
    }

    void setClock(Clock* c, double pts, int serial);

    double pts;
    int pos; //unit: second
    ClockType clock_type;
    double max_frame_duration;
    Clock clocks[SyncToNone];
    bool paused;
    float speed;
	bool eof;
};

void AVClockPrivate::setClock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    c->pts = pts;
    c->serial = serial;
    c->last_updated = time;
    c->pts_drift = pts - time;
}

AVClock::AVClock():
    d_ptr(new AVClockPrivate)
{

}

AVClock::~AVClock() {

}

void AVClock::init(ClockType type, int *s)
{
    DPTR_D(AVClock);
    Clock *c = &d->clocks[type];
    c->queue_serial = s;
    d->setClock(c, FORCE_DOUBLE(NAN), -1);
}

void AVClock::pause(bool p)
{
	DPTR_D(AVClock);
	d->paused = p;
}

float AVClock::speed() const
{
    DPTR_D(const AVClock);
    return d->speed;
}

void AVClock::setSpeed(float s)
{
    DPTR_D(AVClock);
    d->speed = av_clipf(s, 0.5, 12.0);
}

double AVClock::value() const
{
    DPTR_D(const AVClock);
    return value(d->clock_type);
}

double AVClock::value(ClockType type) const
{
    DPTR_D(const AVClock);
    const Clock *c = &d->clocks[type];
	if (d->eof)
		return c->pts;
    if (*c->queue_serial != c->serial)
        return FORCE_DOUBLE(NAN);
    if (d->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        return c->pts_drift + time - (time - c->last_updated) * (1.0f - d->speed);
    }
}

double AVClock::diffToMaster(ClockType type) const
{
    return value(type) - value();
}

void AVClock::updateValue(ClockType type, double pts, int serial)
{
    DPTR_D(AVClock);

    Clock *c = &d->clocks[type];
    d->setClock(c, pts, serial);
}

void AVClock::updateClock(ClockType dst, ClockType src)
{
	DPTR_D(AVClock);

	Clock *c_d = &d->clocks[dst];
	Clock *c_s = &d->clocks[src];
	d->setClock(c_d, c_s->pts, c_s->serial);
}

Clock* AVClock::clock(ClockType type)
{
    DPTR_D(AVClock);
    return &d->clocks[type];
}

ClockType AVClock::type() const
{
    DPTR_D(const AVClock);
    return d->clock_type;
}

void AVClock::setClockType(ClockType type)
{
    DPTR_D(AVClock);
    d->clock_type = type;
}

void AVClock::setMaxDuration(double duration)
{
    DPTR_D(AVClock);
    d->max_frame_duration = duration;
}

double AVClock::maxDuration() const
{
    return d_func()->max_frame_duration;
}

int * AVClock::serialAddr(ClockType t)
{
	DPTR_D(AVClock);
	return &d->clocks[t].serial;
}

void AVClock::setEof(bool eof)
{
	DPTR_D(AVClock);
	d->eof = eof;
}

NAMESPACE_END
