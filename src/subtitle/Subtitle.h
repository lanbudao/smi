#ifndef SUBTITLE_H
#define SUBTITLE_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "sdk/mediainfo.h"
#include "Packet.h"
#include <vector>

struct AVFrame;
NAMESPACE_BEGIN

enum SubtitleType {
    SubtitleText,
    SubtitlePixmap
};

class SubtitleFramePrivate;
class SubtitleFrame
{
    DPTR_DECLARE_PRIVATE(SubtitleFrame);
public:
    SubtitleFrame();
    ~SubtitleFrame();

    bool isValid() const { return start < stop; }
    bool operator < (const SubtitleFrame &f) const { return stop < f.stop; }
    inline bool operator < (double t) const { return stop < t; }
    void push_back(const std::string &text);
    const std::vector<std::string> &texts() const;
    const std::vector<AVFrame*> &images() const;
    void reset();

    double start, stop;
    SubtitleType type;

private:
    DPTR_DECLARE(SubtitleFrame);
};

class SubtitlePrivate;
class Subtitle
{
    DPTR_DECLARE_PRIVATE(Subtitle)
public:
    Subtitle();
    ~Subtitle();

    bool enabled() const;
    void setEnabled(bool b);
    void setFile(const std::string &name);
    void setCodec(const std::string &codec);
    void load();
    void seek(double pos, double incr, SeekType type);
    bool processHeader(MediaInfo *info);
    bool processLine(Packet *pkt);

    void setTimestamp(double t);
    SubtitleFrame frame();

protected:

private:
    DPTR_DECLARE(Subtitle)
};

NAMESPACE_END
#endif //SUBTITLE_H
