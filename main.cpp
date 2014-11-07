#include <QApplication>
#include "novenatestwindow.h"

#ifdef Q_WS_X11
void qt_x11_wait_for_window_manager(QWidget *widget);
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NovenaTestWindow w;

    /* Hack to disable screen blanking */
    system("xset s off; xset -dpms; xset s noblank");

    w.show();
    
#ifdef Q_WS_X11
    qt_x11_wait_for_window_manager(&w);
#endif

    return a.exec();
}
