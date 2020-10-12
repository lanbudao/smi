#ifndef FRAME_H
#define FRAME_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "ByteArray.h"
#include <vector>
#include <memory>

typedef struct AVFrame AVFrame;

NAMESPACE_BEGIN

class FramePrivate;
class Frame
{
    DPTR_DECLARE_PRIVATE(Frame)
public:
    virtual ~Frame();
    Frame(const Frame &other);
    Frame &operator = (const Frame &other);

    int serial() const;
    void setSerial(int s);

    /**
     * @brief A decoded frame can be packed or planar. Packed frame has only one plane but Planar frame
     * may more than one plane.
     * For audio, plane count equals channel count
     * For video, RGB format is 1, YUV420p format is 3
     * @return
     */
    int planeCount() const;

    /**
     * @brief For audio, channel count == plane count
     * For video, channel count >= plane count
     * @return
     */
    virtual int channelCount() const;

    /**
     * @brief For video, it is the size of each picture lien
     * For audio, it is the whole size of plane
     * @param plane
     * @return
     */
    int bytesPerLine(int plane = 0) const;

    const int *lineSize() const;

    ByteArray frameData();

    uchar * const *datas() const;

    ByteArray data(int plane = 0) const;

    uchar* bits(int plane);

    const uchar *constBits(int plane) const;

    void setBits(uchar *b, int plane = 0);
    void setBits(const std::vector<uchar *> &bits);
    void setBits(uchar *slice[]);

    void setBytesPerLine(int lineSize, int plane);
    void setBytesPerLine(int stride[]);

    double timestamp() const;
    void setTimestamp(double t);
    double pos() const;
    void setPos(double pos);
	void setMetaData(const std::string &key, const std::string &value);

    AVFrame* frame();
protected:
    Frame(FramePrivate *d);
    DPTR_DECLARE(Frame)
};

NAMESPACE_END
#endif //FRAME_H
