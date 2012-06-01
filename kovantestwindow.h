#ifndef KOVANTESTWINDOW_H
#define KOVANTESTWINDOW_H

#include <QMainWindow>
#include "kovantestengine.h"

namespace Ui {
class KovanTestWindow;
}

class KovanTestWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit KovanTestWindow(QWidget *parent = 0);
    ~KovanTestWindow();
    void setStatusText(QString *message);
    void setProgressBar(double progress);
    
private:
    Ui::KovanTestWindow *ui;
    KovanTestEngine *engine;
};

#endif // KOVANTESTWINDOW_H
