#include "qprocrender.h"
#include "qprocplayer.h"
#include <QTimer>
#include <QApplication>
#include <QScreen>

QProcRender::QProcRender(QWidget* parent):
    QOpenGLWidget (parent),
    player(nullptr)
{

}

QProcRender::~QProcRender()
{
}

void QProcRender::setSource(QProcPlayer * p)
{
    player = p;
}

void QProcRender::initializeGL()
{
}

void QProcRender::resizeGL(int w, int h)
{
    double dpi = devicePixelRatio();
    player->setVideoRender(this->width() * dpi, this->height() * dpi, this);
}

void QProcRender::paintGL()
{
    player->renderVideo();
}
