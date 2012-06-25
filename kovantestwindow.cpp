#include <QDebug>
#import <QScrollBar>
#import <QCleanlooksStyle>
#import <QStyle>

#include "kovantestwindow.h"
#include "ui_kovantestwindow.h"

class BiggerScrollbar : public QCleanlooksStyle {

public:
	int pixelMetric(PixelMetric metric, const QStyleOption *, const QWidget *pWidget = 0) const
	{
		if ( metric == PM_ScrollBarExtent )
		{
		   return 50;
		}
		return  QWindowsStyle::pixelMetric( metric, 0, pWidget );
	}
};

KovanTestWindow::KovanTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KovanTestWindow)
{
	QApplication::setStyle(new BiggerScrollbar);

	ui->setupUi(this);

	// Start on the "Start Screen"
	ui->startScreen->setVisible(true);
	ui->mainScreen->setVisible(false);
	ui->debugScreen->setVisible(false);

	ui->debugActiveLabel->setVisible(false);
	ui->selectTestButton->setVisible(false);

#ifdef linux
	showFullScreen();
#endif

    engine = new KovanTestEngine(this);
    engine->loadAllTests();

    connect(ui->startTestsButton, SIGNAL(clicked(bool)),
            this, SLOT(startTests()));

	connect(ui->debugMode1, SIGNAL(clicked(bool)),
			this, SLOT(debugMode1Clicked()));
	connect(ui->debugMode2, SIGNAL(clicked(bool)),
			this, SLOT(debugMode2Clicked()));
	connect(ui->debugMode3, SIGNAL(clicked(bool)),
			this, SLOT(debugMode3Clicked()));

	connect(ui->runSelectedTestsButton, SIGNAL(clicked()),
			this, SLOT(debugRunSelectedItems()));

	connect(ui->testSelectionButton, SIGNAL(clicked()),
			this, SLOT(moveToDebugScreen()));

	serialLabelString.sprintf("Serial number: %s",
							  (const char *)(engine->serialNumber().toAscii()));

	// Start the secret debug code at stage 0
	sequencePosition = 0;
}

void KovanTestWindow::debugItemPressed(QListWidgetItem *)
{
	if (ui->testListWidget->selectedItems().count())
		ui->runSelectedTestsButton->setEnabled(true);
	else
		ui->runSelectedTestsButton->setEnabled(false);
}

void KovanTestWindow::debugRunSelectedItems()
{
	QList<QListWidgetItem *>selected = ui->testListWidget->selectedItems();
	QList<KovanTest *>testsToRun;

	int i;
	for (i=0; i<selected.count(); i++) {
		QVariant v = selected.at(i)->data(Qt::UserRole);
		KovanTest *t = reinterpret_cast<KovanTest*>(v.value<void*>());
		testsToRun.append(t);
	}

	moveToMainScreen();
	engine->runSelectedTests(testsToRun);
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
	else if (SEQUENCE[0] == c)
		sequencePosition = 1;
	else
		sequencePosition = 0;

	if (sequencePosition >= sizeof(SEQUENCE)-1)
		moveToDebugScreen();
}

void KovanTestWindow::moveToDebugScreen()
{
	ui->debugActiveLabel->setVisible(true);
	ui->selectTestButton->setVisible(true);
	ui->debugMode1->setVisible(false);
	ui->debugMode2->setVisible(false);
	ui->debugMode3->setVisible(false);
	ui->testSelectionButton->setVisible(false);
	ui->errorFailLabel->setVisible(false);
	ui->errorLabel->setVisible(false);
	ui->passLabel->setVisible(false);

	ui->runSelectedTestsButton->setEnabled(false);

	sequencePosition = 0;
	engine->setDebug(true);

	ui->startScreen->setVisible(false);
	ui->debugScreen->setVisible(true);

	QList<KovanTest *>tests = engine->allTests();
	int i;
	ui->testListWidget->clear();
	for (i=0; i<tests.count(); i++) {
		QListWidgetItem *item = new QListWidgetItem(*(tests.at(i)->testName()));
		QVariant v = QVariant::fromValue<void*>(tests.at(i));
		item->setData(Qt::UserRole, v);
		item->setSizeHint(QSize(320,30));
		ui->testListWidget->addItem(item);
		connect(ui->testListWidget, SIGNAL(itemPressed(QListWidgetItem *)),
				this, SLOT(debugItemPressed(QListWidgetItem *)));
	}
	ui->testListWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
}

void KovanTestWindow::moveToMainScreen()
{
	ui->errorLabel->setVisible(false);
	ui->errorFailLabel->setVisible(false);
	ui->passLabel->setVisible(false);
	ui->testSelectionButton->setVisible(false);
	ui->progressBar->setVisible(true);
	ui->progressLabel->setVisible(true);

	ui->mainScreen->setVisible(true);
	ui->debugScreen->setVisible(false);
	ui->startScreen->setVisible(false);

	QString str("");
	addTestLog(str);
	QScrollBar *vert = ui->testLog->verticalScrollBar();
	vert->setValue(vert->maximum());
}

void KovanTestWindow::startTests()
{
	moveToMainScreen();
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
	ui->errorLabel->setText(errorString);
}

void KovanTestWindow::finishTests(bool successful)
{
	ui->progressLabel->setVisible(false);
	ui->progressBar->setVisible(false);

	if (successful) {
		ui->passLabel->setVisible(true);
	}
	else {
		ui->errorLabel->setVisible(true);
		ui->errorFailLabel->setVisible(true);
	}

	if (engine->debugModeOn()) {
		ui->testSelectionButton->setVisible(true);
	}
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

KovanTestWindow::~KovanTestWindow()
{
	delete ui;
}
