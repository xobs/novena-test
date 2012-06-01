#include <QtGui/QApplication>
#include "kovantestwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KovanTestWindow w;
    w.show();
    
    return a.exec();
}
