#include "sdk/filter/subtitlefilter.h"
#include "Filter_p.h"
#include "player.h"
#include "renderer/subtitlerender.h"
#include "subtitle//Subtitle.h"
#include "VideoFrame.h"

NAMESPACE_BEGIN

class SubtitleFilterPrivate : public RenderFilterPrivate
{
public:
    SubtitleFilterPrivate() :
        player(nullptr),
        pos(1.0)
    {
        subtitle = new Subtitle();
        render.setFontFile("E:/ht.ttf");
    }
    ~SubtitleFilterPrivate()
    {
        delete subtitle;
    }

public:
    Player *player;
    std::string fontfile;
    std::string filename;
    float pos;
    SubtitleRender render;
    Subtitle *subtitle;
};

SubtitleFilter::SubtitleFilter(Player *player, int track, const std::string &font):
    RenderFilter(new SubtitleFilterPrivate)
{
    if (player) {
        setPlayer(player);
        if (track >= 0)
            player->setSubtitleStream(track);
    }
    setFont(font);
}

SubtitleFilter::~SubtitleFilter()
{

}

void SubtitleFilter::setFont(const std::string & font)
{
    DPTR_D(SubtitleFilter);
    d->fontfile = font;
    d->render.setFontFile(font);
}

void SubtitleFilter::setPlayer(Player * player)
{
    DPTR_D(SubtitleFilter);
    d->player = player;
    auto process_line = [d](Packet* pkt)->
        void {d->subtitle->processLine(pkt);};
    auto process_header = [d](MediaInfo *info)->
        void {d->subtitle->processHeader(info); };
    auto process_seek = [d](double pos, double incr, SeekType type)->
        void {d->subtitle->seek(pos, incr, type); };
    player->setSubtitlePacketCallback(process_line);
    player->setSubtitleHeaderCallback(process_header);
    player->setSeekRequestCallback(process_seek);
}

void SubtitleFilter::setFile(const std::string & filename)
{
    DPTR_D(SubtitleFilter);
    d->filename = filename;
    d->subtitle->setFile(filename);
}

void SubtitleFilter::setPos(float pos)
{
    DPTR_D(SubtitleFilter);
    d->pos = pos;
}

void SubtitleFilter::setTimestamp(double t)
{
    DPTR_D(SubtitleFilter);
    d->subtitle->setTimestamp(t);
}

void SubtitleFilter::process(MediaInfo * info, VideoFrame * frame)
{
    DPTR_D(SubtitleFilter);
    d->subtitle->setTimestamp(frame->timestamp());
    SubtitleFrame f = d->subtitle->frame();
    if (!f.isValid())
        return;
    if (f.type == SubtitleText) {
        std::string text;
        const std::vector<std::string> &texts = f.texts();
        for (int i = 0; i < texts.size(); ++i) {
            text.append(texts.at(i));
            if (i != texts.size() - 1)
                text.append("\n");
        }
        d->render.drawText(text);
    }
    else if (f.type == SubtitlePixmap) {
        //TODO
        d->render.drawPixmap();
    }
}

NAMESPACE_END