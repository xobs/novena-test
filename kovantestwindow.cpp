#include "kovantestwindow.h"
#include "ui_kovantestwindow.h"
#include <QDebug>
#import <QScrollBar>

KovanTestWindow::KovanTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KovanTestWindow)
{
    ui->setupUi(this);

	ui->startScreen->setVisible(true);

	ui->mainLayout->setVisible(false);
	ui->errorLabel->setVisible(false);
	ui->errorFailLabel->setVisible(false);

	ui->debugActiveLabel->setVisible(false);

#ifdef linux
	showFullScreen();
#endif
    engine = new KovanTestEngine(this);
    engine->loadAllTests();

    connect(ui->startTestsButton, SIGNAL(clicked(bool)),
            this, SLOT(startTests()));

	sequencePosition = 0;
	connect(ui->debugMode1, SIGNAL(clicked(bool)),
			this, SLOT(debugMode1Clicked()));
	connect(ui->debugMode2, SIGNAL(clicked(bool)),
			this, SLOT(debugMode2Clicked()));
	connect(ui->debugMode3, SIGNAL(clicked(bool)),
			this, SLOT(debugMode3Clicked()));

}

KovanTestWindow::~KovanTestWindow()
{
    delete ui;
}

void KovanTestWindow::debugMode1Clicked()
{
	advanceDebugSequence('1');
}

void KovanTestWindow::debugMode2Clicked()
{
	advanceDebugSequence('2');
}

void KovanTestWindow::debugMode3Clicked()
{
	advanceDebugSequence('3');
}

void KovanTestWindow::advanceDebugSequence(const char c)
{
	if (SEQUENCE[sequencePosition] == c)
		sequencePosition++;
	else
		sequencePosition = 0;

	qDebug() << "Sequence position:" << sequencePosition;

	if (sequencePosition >= sizeof(SEQUENCE)-1) {
		ui->debugActiveLabel->setVisible(true);
		sequencePosition = 0;
		engine->setDebug(true);
	}
}

void KovanTestWindow::startTests()
{
	ui->startScreen->setVisible(false);
	ui->mainLayout->setVisible(true);
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

void KovanTestWindow::setErrorString(QString &message)
{
	errorString = message;

	ui->progressLabel->setVisible(false);
	ui->progressBar->setVisible(false);

	ui->errorLabel->setText(errorString);
	ui->errorLabel->setVisible(true);
	ui->errorFailLabel->setVisible(true);
}

void KovanTestWindow::addTestLog(QString &message)
{
	QScrollBar *vert = ui->testLog->verticalScrollBar();
	bool shouldScroll = false;
	if (vert->value() == vert->maximum())
		shouldScroll = true;

    ui->testLog->appendHtml(message);

	if (shouldScroll)
		vert->setValue(vert->maximum());
}

