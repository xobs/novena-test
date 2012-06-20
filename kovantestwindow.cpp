#include "kovantestwindow.h"
#include "ui_kovantestwindow.h"
#include <QDebug>

KovanTestWindow::KovanTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KovanTestWindow)
{
    ui->setupUi(this);
    showFullScreen();
    engine = new KovanTestEngine(this);
    engine->loadAllTests();
    engine->runAllTests();
}

KovanTestWindow::~KovanTestWindow()
{
    delete ui;
}

void KovanTestWindow::setStatusText(QString *message)
{
    ui->statusLabel->setText(*message);
    return;
}

void KovanTestWindow::setProgressBar(double progress)
{
    ui->progressBar->setValue(progress*100);
    return;
}
