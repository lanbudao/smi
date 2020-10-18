#include "mainwindow.h"
#include <QApplication>

int xabs(int n)
{
    int s = n >> (sizeof(int) * 8 - 1); // equals int s = n < 0 ? -1 : 0
    int t = n ^ s;
    return t - s;
}
int vtod(int x)
{

    int i = (x - 10) >> (sizeof(int) * 8 - 1); // equals int i = x < 10 ? -1 : 0;
    // -x = ~x + 1
    // -1 & x = x
    return x + 55 - (((x-10) >> 15) & 7);
}

int main(int argc, char *argv[])
{
#if 0
    int x = 0x8f;
    int j = -1 & x;
    int array[16];
    for (int i = 0; i < 16; ++i) {
        int temp = vtod(i);
        array[i] = temp;
        printf("%c ", temp);
    }
    int xxx = vtod(130);
    int aaa = xabs(-123);
#endif
    
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
