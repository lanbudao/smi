#ifndef AVCLOCK_H
#define AVCLOCK_H

#include "sdk/global.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

typedef struct Clock
{
    double pts = 0;           /* clock base */
    double pts_drift = 0;     /* clock base minus time at which we updated the clock */
    double last_updated = 0;
    int serial = -1;           /* clock is based on a packet with this serial */
    int *queue_serial = nullptr;    /* pointer to the current packet queue serial, used for obsolete clock detection */
    void init() {
        pts = 0; pts_drift = 0; last_updated = 0; serial = -1;
    }
} Clock;

class AVClockPrivate;
class AVClock
{
    DPTR_DECLARE_PRIVATE(AVClock)
public:
    AVClock();
    ~AVClock();

    void init(ClockType type, int *s);
	void pause(bool p);
    float speed() const;
    void setSpeed(float s);
	/**
	 * @brief 
	 * @return the pts of the master clock
	 */
    double value() const;
    double value(ClockType type) const;
    double diffToMaster(ClockType type) const;
    void updateValue(ClockType type, double pts, int serial);
	void updateClock(ClockType dst, ClockType src);

    Clock* clock(ClockType type);

    ClockType type() const;
    void setClockType(ClockType type);

    void setMaxDuration(double d);
    double maxDuration() const;

	int* serialAddr(ClockType t);

	void setEof(bool eof);

private:
    DPTR_DECLARE(AVClock)
};

NAMESPACE_END
#endif //AVCLOCK_H
