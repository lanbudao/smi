#ifndef PACKET_H
#define PACKET_H

#include "Frame.h"
#include "Frame_p.h"

NAMESPACE_BEGIN

Frame::Frame(FramePrivate *d)
    :d_ptr(d)
{

}

Frame::Frame(const Frame &other)
    :d_ptr(other.d_ptr)
{

}

Frame::~Frame()
{

}

Frame &Frame::operator=(const Frame &other) {
    d_ptr = other.d_ptr;
    return *this;
}

int Frame::serial() const
{
    return d_func()->serial;
}

void Frame::setSerial(int s)
{
    d_func()->serial = s;
}

int Frame::planeCount() const {
    DPTR_D(const Frame);
    return d->planes.size();
}

int Frame::channelCount() const {
    return planeCount();
}

int Frame::bytesPerLine(int plane) const {
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount())
        return 0;
    return d->line_sizes[plane];
}

const int *Frame::lineSize() const
{
    DPTR_D(const Frame);
    return d->line_sizes.data();
}

ByteArray Frame::frameData() {
    DPTR_D(Frame);
    return d->data;
}

uchar * const *Frame::datas() const
{
    DPTR_D(const Frame);
    return d->planes.data();
}

ByteArray Frame::data(int plane) const {
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount())
        return ByteArray();
    return ByteArray((char*)d->planes[plane], bytesPerLine(plane));
}

uchar *Frame::bits(int plane) {
    DPTR_D(Frame);
    if (plane < 0 || plane >= planeCount())
        return nullptr;
    return d->planes[plane];
}

const uchar* Frame::constBits(int plane) const {
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount())
        return nullptr;
    return d->planes[plane];
}

void Frame::setBits(uchar *b, int plane) {
    DPTR_D(Frame);
    if (plane < 0 || plane >= planeCount())
        return;
    d->planes[plane] = b;
}

void Frame::setBits(const vector<uchar *> &bits) {
    DPTR_D(Frame);
    const int nb_plane = planeCount();
    d->planes = bits;
    if (d->planes.size() > nb_plane) {
        d->planes.reserve(nb_plane);
        d->planes.resize(nb_plane);
    }
}

void Frame::setBits(uchar *slice[]) {
    for (int i = 0; i < planeCount(); ++i) {
        setBits(slice[i], i);
    }
}

void Frame::setBytesPerLine(int lineSize, int plane) {
    DPTR_D(Frame);
    if (plane < 0 || plane >= planeCount())
        return;
    d->line_sizes[plane] = lineSize;
}

void Frame::setBytesPerLine(int stride[]) {
    for (int i = 0; i < planeCount(); ++i) {
        setBytesPerLine(stride[i], i);
    }
}

double Frame::timestamp() const {
    DPTR_D(const Frame);
    return d->timestamp;
}

void Frame::setTimestamp(double t) {
    DPTR_D(Frame);
    d->timestamp = t;
}

double Frame::pos() const
{
    DPTR_D(const Frame);
    return d->pos;
}

void Frame::setPos(double p)
{
    DPTR_D(Frame);
    d->pos = p;
}

void Frame::setMetaData(const std::string &key, const std::string &value)
{
	DPTR_D(Frame);
	if (!value.empty())
		d->metadata.insert(std::make_pair(key, value));
	else
        d->metadata.erase(key);
}

AVFrame *Frame::frame()
{
    return d_func()->frame;
}

NAMESPACE_END
#endif //PACKET_H
