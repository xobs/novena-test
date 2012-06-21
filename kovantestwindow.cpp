#include "kovantestwindow.h"
#include "ui_kovantestwindow.h"
#include <QDebug>
#import <QScrollBar>

KovanTestWindow::KovanTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KovanTestWindow)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    ui->progressLabel->setVisible(false);
    ui->testLog->setVisible(false);
    ui->statusLabel->setVisible(false);
    showFullScreen();
    engine = new KovanTestEngine(this);
    engine->loadAllTests();

    connect(ui->startTestsButton, SIGNAL(clicked(bool)),
            this, SLOT(startTests()));
}

KovanTestWindow::~KovanTestWindow()
{
    delete ui;
}

void KovanTestWindow::startTests()
{
    ui->progressBar->setVisible(true);
    ui->progressLabel->setVisible(true);
    ui->testLog->setVisible(true);
    ui->statusLabel->setVisible(true);

    ui->startTestsButton->setVisible(false);
    engine->runAllTests();
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

void KovanTestWindow::setProgressText(QString &message)
{
    ui->progressLabel->setText(message);
}

void KovanTestWindow::addTestLog(QString &message)
{
    ui->testLog->appendHtml(message);
    ui->testLog->verticalScrollBar()->setValue(ui->testLog->verticalScrollBar()->maximum());
}
