#include "mainwindow.h"
#include "qprocplayer.h"
#include "qprocrender.h"
#include <QOpenGLWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QHBoxLayout>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    this->resize(400, 300);
    this->setWindowTitle(QString::fromUtf8("ffproc"));
    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QString::fromUtf8("menuBar"));
    this->setMenuBar(menuBar);
    mainToolBar = new QToolBar(this);
    mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
    this->addToolBar(mainToolBar);
    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
    this->setCentralWidget(centralWidget);
    statusBar = new QStatusBar(this);
    statusBar->setObjectName(QString::fromUtf8("statusBar"));
    this->setStatusBar(statusBar);
    QHBoxLayout *layout = new QHBoxLayout;
    player = new QProcPlayer(this);
    glRender = new QProcRender();
    layout->addWidget(glRender);
    centralWidget->setLayout(layout);
    glRender->setSource(player);
    player->setMedia("E:/media/Æ½·²Ö®Â·.mp4");
    player->play();
}

MainWindow::~MainWindow()
{
    delete player;
}

void MainWindow::showEvent(QShowEvent * event)
{
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QSize s = e->size();
    QSize ss = centralWidget->size();
    //glRender->resize(this->centralWidget->size());
}
