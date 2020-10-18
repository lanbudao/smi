#include "mainwindow.h"
#include "qprocplayer.h"
#include "qprocrender.h"
#include <QOpenGLWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    this->resize(400, 300);
    this->setWindowTitle(QString::fromUtf8("ffproc"));
    menuBar = new QMenuBar(this);
    menuBar->setObjectName(QString::fromUtf8("menuBar"));
    fileMenu = new QMenu(QString::fromUtf8("File"));
    openfileAction = new QAction(QString::fromUtf8("Open"));
    fileMenu->addAction(openfileAction);
    stopAction = new QAction(QString::fromUtf8("Stop"));
    fileMenu->addAction(stopAction);
    menuBar->addMenu(fileMenu);
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

    connect(openfileAction, &QAction::triggered, [this](bool checked) {
        QString file = QFileDialog::getOpenFileName(this,
            tr("Open a media file"),
            tr(""));
        if (!file.isEmpty()) {
            player->setMedia(file.toUtf8().data());
            player->play();
        }
    });
    connect(stopAction, &QAction::triggered, [this](bool checked) {
        player->stop();
    });
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
