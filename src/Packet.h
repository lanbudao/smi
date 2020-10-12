#ifndef PACKET_H
#define PACKET_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "ByteArray.h"

struct AVPacket;

NAMESPACE_BEGIN

class PacketPrivate;
class Packet
{
    DPTR_DECLARE_PRIVATE(Packet)
public:
    Packet();
    ~Packet();
    Packet(const Packet& other);
    Packet &operator = (const Packet &other);

    const AVPacket *asAVPacket() const;

    static Packet fromAVPacket(const AVPacket *packet, double time_base);

    bool isEOF() const;
    static Packet createEOF();
    inline bool isValid() const;
    bool isFlush() const;
    static Packet createFlush();

    bool containKeyFrame, isCorrupted;
    double pts, dts, duration;
    int64_t pos;
    ByteArray data;
    mutable int serial;

    AVPacket *avpacket();

private:
    DPTR_DECLARE(Packet)
};

bool Packet::isValid() const
{
    if (data.isEmpty())
        return false;
    return !isCorrupted && data.size() > 0 && pts >= 0 && duration >= 0;
}

NAMESPACE_END
#endif //PACKET_H
