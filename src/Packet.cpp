#include "Packet.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

NAMESPACE_BEGIN

class PacketPrivate
{
public:
    PacketPrivate()
    {
        av_init_packet(&avpkt);
    }
    ~PacketPrivate()
    {
        av_packet_unref(&avpkt);
    }

    mutable AVPacket avpkt;
};

Packet::Packet():
    d_ptr(new PacketPrivate),
    containKeyFrame(false),
    isCorrupted(false),
    pts(-1),
    dts(-1),
    duration(-1),
    pos(-1),
    serial(-1)
{

}

Packet::~Packet()
{

}

Packet::Packet(const Packet &other)
{
    d_ptr = other.d_ptr;
    containKeyFrame = other.containKeyFrame;
    isCorrupted = other.isCorrupted;
    pts = other.pts;
    duration = other.duration;
    dts = other.dts;
    pos = other.pos;
    data = other.data;
    serial = other.serial;
}

Packet &Packet::operator =(const Packet &other)
{
    if (this == &other)
        return *this;
    d_ptr = other.d_ptr;
    containKeyFrame = other.containKeyFrame;
    isCorrupted = other.isCorrupted;
    pts = other.pts;
    duration = other.duration;
    dts = other.dts;
    pos = other.pos;
    data = other.data;
    serial = other.serial;
    return *this;
}

Packet Packet::fromAVPacket(const AVPacket *packet, double time_base)
{
    Packet pkt;

    pkt.pos = packet->pos;
    pkt.containKeyFrame = !!(packet->flags & AV_PKT_FLAG_KEY);
    pkt.isCorrupted = !!(packet->flags & AV_PKT_FLAG_CORRUPT);

    /*Set pts*/
    if (packet->pts != AV_NOPTS_VALUE) {
        pkt.pts = packet->pts * time_base;
    } else if (packet->dts != AV_NOPTS_VALUE) {
        pkt.pts = packet->dts * time_base;
    } else {
        pkt.pts = 0;
    }

    /*Set dts*/
    if (packet->dts != AV_NOPTS_VALUE) {
        pkt.dts = packet->dts * time_base;
    } else {
        pkt.dts = pkt.pts;
    }

    if (pkt.pts < 0)
        pkt.pts = 0;
    if (pkt.dts < 0)
        pkt.dts = 0;

    /*duration*/
    pkt.duration = packet->duration * time_base;
    if (pkt.duration < 0)
        pkt.duration = 0;

    AVPacket *p = pkt.avpacket();
    av_packet_ref(p, packet);  //properties are copied internally
    p->pts = int64_t(pkt.pts * 1000.0);
    p->dts = int64_t(pkt.dts * 1000.0);
    p->duration = int(pkt.duration * 1000.0);

    pkt.data.setData((char*)p->data, p->size);

    return pkt;
}

bool Packet::isEOF() const
{
    if (data.isEmpty())
        return false;
    return !memcmp(data.constData(), "eof", data.size()) && pts < 0.0 && dts < 0.0;
}

Packet Packet::createEOF()
{
    static Packet pkt;
    pkt.data = ByteArray("eof");
    return pkt;
}

bool Packet::isFlush() const
{
    if (data.isEmpty())
        return false;
    return !memcmp(data.constData(), "flush", data.size()) && pts < 0.0 && dts < 0.0;
}

Packet Packet::createFlush()
{
    static Packet pkt;
    pkt.data = ByteArray("flush");
    return pkt;
}

AVPacket *Packet::avpacket()
{
    DPTR_D(Packet);
    return &d->avpkt;
}

const AVPacket *Packet::asAVPacket() const
{
    DPTR_D(const Packet);
    AVPacket *p = &d->avpkt;
    p->pts = int64_t(pts * 1000.0);
    p->dts = int64_t(dts * 1000.0);
    p->duration = int(duration * 1000.0);
    p->pos = pos;
    if (isCorrupted)
        p->flags |= AV_PKT_FLAG_CORRUPT;
    if (containKeyFrame)
        p->flags |= AV_PKT_FLAG_KEY;
    if (!data.isEmpty()) {
        p->data = (uint8_t *)data.constData();
        p->size = data.size();
    }
    return p;
}

NAMESPACE_END
