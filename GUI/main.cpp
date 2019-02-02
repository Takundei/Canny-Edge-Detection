#include "cannyedge.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CannyEdge w;
    w.show();

    return a.exec();
}
