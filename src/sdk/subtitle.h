#ifndef SUBTITLE_H
#define SUBTITLE_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "sdk/mediainfo.h"
#include "Packet.h"
#include <vector>

NAMESPACE_BEGIN

class SubtitleFrame;
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

    void processSeek();
    void setTimestamp(double t, int w, int h);
    SubtitleFrame* frame();

protected:

private:
    DPTR_DECLARE(Subtitle)
};

NAMESPACE_END
#endif //SUBTITLE_H
