#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include "Packet.h"
#include "util/BlockQueue.h"
#include "sdk/DPTR.h"

NAMESPACE_BEGIN

class PacketQueuePrivate;
class PacketQueue: public BlockQueue<Packet>
{
    DPTR_DECLARE_PRIVATE(PacketQueue);
public:
    PacketQueue();
    ~PacketQueue() PU_DECL_OVERRIDE;

    double duration() const;//unused temporarily
    int serial() const;
    int* serialAddr();

    bool prepared() const;
    void setRealTime(bool r);
	void setBufferMode(BufferMode mode);
	BufferMode bufferMode() const;
	/**
	 * @brief setBufferValue
	 * Ensure the buffered msecs/bytes/packets in queue is at least the given value
	 * @param value BufferBytes: bytes, BufferTime: msecs, BufferPackets: packets count
	 */
	void setBufferValue(int64_t value);
	int64_t bufferValue() const;
	/**
	 * @brief setBufferMax
	 * stop buffering if max value reached. Real value is bufferValue()*bufferMax()
	 * @param max the ratio to bufferValue(). always >= 1.0
	 */
	void setBufferMax(double max);
	double bufferMax() const;
	/**
	 * @brief buffered
	 * Current buffered value in the queue
	 */
	int64_t buffered() const;
	bool isBuffering() const;
	/**
	 * @brief bufferProgress
	 * How much of the data buffer is currently filled, from 0.0 (empty) to 1.0 (enough or full).
	 * bufferProgress() can be less than 1 if not isBuffering();
	 * @return Percent of buffered time, bytes or packets.
	 */
	float bufferProgress() const;
	/**
	 * @brief bufferSpeed
	 * Depending on BufferMode, the result is delta_pts/s, packets/s or bytes/s
	 * @return
	 */
	float bufferSpeed() const;
	float bufferSpeedInBytes() const;

	bool checkEnough() const override;
	bool checkFull() const override;
protected:
	void onEnqueue(const Packet &t);
	void onDequeue(const Packet &t);

private:
    DPTR_DECLARE(PacketQueue);
};

NAMESPACE_END
#endif //PACKETQUEUE_H
