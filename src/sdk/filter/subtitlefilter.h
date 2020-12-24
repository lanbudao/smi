#ifndef SUBTITLE_FILTER_H
#define SUBTITLE_FILTER_H

#include "sdk/global.h"
#include "sdk/DPTR.h"
#include "sdk/filter/Filter.h"

NAMESPACE_BEGIN

class Player;
class SubtitleFilterPrivate;
class Subtitle;
class SMI_EXPORT SubtitleFilter: public RenderFilter
{
    DPTR_DECLARE_PRIVATE(SubtitleFilter)
public:
    SubtitleFilter(Player *player = nullptr, int track = -1, const std::string &font = "");
    ~SubtitleFilter();

    void setFont(const std::string &font);

    /**
     * @brief there are two ways to load subtitle
     * 1, from internal subtitle tracks
     * 2, from external subtitle files
     */
    void setPlayer(Player* player);
    void setFile(const std::string &filename);

    /**
     * @brief set the pos of subtitle to render, 0~1.0f.
     * default value is 1.0f, subtitle render at the bottom of screen.
     */
    void setPos(float pos);

    void setTimestamp(double t);

protected:
    void process(MediaInfo* info, VideoFrame* frame = nullptr);

};

NAMESPACE_END
#endif
