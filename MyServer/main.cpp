#include "myserver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyServer w(2222);
    w.show();

    return a.exec();
}
