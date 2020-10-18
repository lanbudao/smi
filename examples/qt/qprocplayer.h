#ifndef QPROCPLAYER_H
#define QPROCPLAYER_H

#include <QObject>
#include <QVector>
#include "sdk/global.h"

namespace FFPROC {
class Player;
}
using namespace FFPROC;
class QProcPlayer: public QObject
{
    Q_OBJECT
public:
    QProcPlayer(QObject* parent = nullptr);
    ~QProcPlayer();

    void setMedia(const std::string& url);
    void setVideoRender(int w, int h, QObject* render);
    void play();
    void pause(bool p);
    void stop();
    void renderVideo();
signals:
    void mediaStatusChanged(MediaStatus s);

private:
    Player* player;
};

#endif // QPROCPLAYER_H
