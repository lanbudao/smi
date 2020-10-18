#include "qprocplayer.h"
#include "sdk/player.h"
#include <QCoreApplication>
#include <QWidget>
#include <QVariant>

QProcPlayer::QProcPlayer(QObject* parent):
    QObject(parent),
    player(nullptr)
{

    player = new Player();
}

QProcPlayer::~QProcPlayer()
{
    player->stop();
    delete player;
    player = nullptr;
}

void QProcPlayer::setMedia(const std::string &url)
{
    player->setResampleType(ResampleSoundtouch);
    player->setMedia(url);
}

void QProcPlayer::setVideoRender(int w, int h, QObject* render)
{
    player->setVideoRenderer(w, h, render);
    player->setRenderCallback([this](void* opaque) {
        auto render = reinterpret_cast<QWidget*>(opaque);
        if (!render->isWidgetType()) {
            QCoreApplication::instance()->postEvent(render, new QEvent(QEvent::UpdateRequest));
        }
        class QUpdateLaterEvent final : public QEvent {
        public:
            explicit QUpdateLaterEvent(const QRegion& paintRegion)
                : QEvent(UpdateLater), m_region(paintRegion)
            {}
            ~QUpdateLaterEvent() {}
            inline const QRegion &region() const { return m_region; }
        protected:
            QRegion m_region;
        };
        QCoreApplication::instance()->postEvent(render, 
            new QUpdateLaterEvent(QRegion(0, 0, render->property("width").toInt(), render->property("height").toInt())));
    });
}

void QProcPlayer::play()
{
    player->setMediaStatusCallback([this](FFPROC::MediaStatus status) {
        emit mediaStatusChanged(status);
        });
    player->prepare();
}

void QProcPlayer::pause(bool p)
{
    player->pause(p);
}

void QProcPlayer::stop()
{
    player->stop();
}

void QProcPlayer::renderVideo()
{
    player->renderVideo();
}
