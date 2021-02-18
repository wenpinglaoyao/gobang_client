#include "gobangboard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    GobangBoard b;
    b.show();

    return a.exec();
}
