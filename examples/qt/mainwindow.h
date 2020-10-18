#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QProcRender;
class QProcPlayer;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent* e);

private:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;
    QProcRender* glRender;
    QProcPlayer* player;
};

#endif // MAINWINDOW_H
