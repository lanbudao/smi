#ifndef QPROCRENDER_H
#define QPROCRENDER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class QProcPlayer;
class QProcRender: public QOpenGLWidget, public QOpenGLFunctions
{
public:
    QProcRender(QWidget* parent = nullptr);
    ~QProcRender();

    void setSource(QProcPlayer* p);

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

private:
    QProcPlayer* player;
};

#endif // QPROCRENDER_H
