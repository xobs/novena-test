#include <QDebug>
#include <QScrollBar>
//#include <QCleanlooksStyle>
#include <QStyle>
#include <QFile>
#include <QDir>

#include "novenatestwindow.h"
#include "ui_novenatestwindow.h"

/*
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
*/

NovenaTestWindow::NovenaTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NovenaTestWindow)
{
    //QApplication::setStyle(new BiggerScrollbar);

    ui->setupUi(this);

    // Start on the "Start Screen"
    ui->startScreen->setVisible(true);
    ui->mainScreen->setVisible(false);
    ui->debugScreen->setVisible(false);

    ui->debugActiveLabel->setVisible(false);
    ui->selectTestButton->setVisible(false);

    ui->startTestsButton->setVisible(false);
    ui->lookingForUSBLabel->setVisible(true);

#ifdef linux
//    showFullScreen();
#endif

    engine = new NovenaTestEngine(this);
    engine->loadAllTests();

    // Wire up all start-screen UI buttons
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

    // Start the secret debug code at stage 0
    sequencePosition = 0;

    openLogFile();

    ui->startTestsButton->setVisible(true);
    ui->lookingForUSBLabel->setVisible(false);

    startTests();
}

void NovenaTestWindow::openLogFile()
{
    QString testDirectory;
    QString testFile;
    QString serial = engine->serialNumber();
    QString mountPoint = "/factory/";
    QDir currentDir(mountPoint);

    qDebug() << "Current dir (mountPoint):" << currentDir.path();
    testDirectory = "novena-logs/";

    if (!currentDir.mkpath(testDirectory)) {
        qDebug() << "Unable to make directory";
        return;
    }

    testFile = testDirectory;
    testFile.append("/");
    testFile.append(serial);
    testFile.append("-");
    testFile.append(QString::number(time(NULL)));
    testFile.append(".html");

    logFile.setFileName(currentDir.absoluteFilePath(testFile));
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Unable to create logfile" << logFile.fileName();
        return;
    }

    logFile.write("<p>Started a new test\n");
    logFile.flush();
    return;
}

void NovenaTestWindow::debugItemPressed(QListWidgetItem *)
{
    if (ui->testListWidget->selectedItems().count())
        ui->runSelectedTestsButton->setEnabled(true);
    else
        ui->runSelectedTestsButton->setEnabled(false);
}

void NovenaTestWindow::debugRunSelectedItems()
{
    QList<QListWidgetItem *>selected = ui->testListWidget->selectedItems();
    QList<NovenaTest *>testsToRun;

    int i;
    for (i=0; i<selected.count(); i++) {
        QVariant v = selected.at(i)->data(Qt::UserRole);
        NovenaTest *t = reinterpret_cast<NovenaTest*>(v.value<void*>());
        testsToRun.append(t);
    }

    moveToMainScreen();
    engine->runSelectedTests(testsToRun);
}

void NovenaTestWindow::debugMode1Clicked()
{
    advanceDebugSequence('1');
}

void NovenaTestWindow::debugMode2Clicked()
{
    advanceDebugSequence('2');
}

void NovenaTestWindow::debugMode3Clicked()
{
    advanceDebugSequence('3');
}

void NovenaTestWindow::advanceDebugSequence(const char c)
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

void NovenaTestWindow::moveToDebugScreen()
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

    QList<NovenaTest *>tests = engine->allTests();
    int i;
    ui->testListWidget->clear();
    for (i=0; i<tests.count(); i++) {
        QListWidgetItem *item = new QListWidgetItem(tests.at(i)->testName());
        QVariant v = QVariant::fromValue<void*>(tests.at(i));
        item->setData(Qt::UserRole, v);
        item->setSizeHint(QSize(320,30));
        ui->testListWidget->addItem(item);
        connect(ui->testListWidget, SIGNAL(itemPressed(QListWidgetItem *)),
                this, SLOT(debugItemPressed(QListWidgetItem *)));
    }
    ui->testListWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
}

void NovenaTestWindow::moveToMainScreen()
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

void NovenaTestWindow::startTests()
{
    if (!logFile.isOpen()) {
        logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        logFile.write("<p>Restarted test\n");
        logFile.flush();
    }

    moveToMainScreen();
    engine->runAllTests();
}

void NovenaTestWindow::setStatusText(QString message)
{
    ui->statusLabel->setText(message);
    return;
}

void NovenaTestWindow::setProgressBar(double progress)
{
    ui->progressBar->setValue(progress*100);
    return;
}

void NovenaTestWindow::setProgressText(QString &message)
{
    ui->progressLabel->setText(message);
}

void NovenaTestWindow::setErrorString(QString &message)
{
    errorString = message;
    ui->errorLabel->setText(errorString);
}

void NovenaTestWindow::finishTests(bool successful)
{
    ui->progressLabel->setVisible(false);
    ui->progressBar->setVisible(false);

    if (successful) {
        ui->passLabel->setVisible(true);
        logFile.write("<p><font color=\"green\">PASS</font>\n");
    }
    else {
        ui->errorLabel->setVisible(true);
        ui->errorFailLabel->setVisible(true);
        logFile.write("<p><font color=\"red\">FAIL</font>\n");
    }

    if (engine->debugModeOn()) {
        ui->testSelectionButton->setVisible(true);
    }

    logFile.close();
}

void NovenaTestWindow::addTestLog(QString &message)
{
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    bool shouldScroll = false;
    if (vert->value() == vert->maximum())
        shouldScroll = true;

    ui->testLog->appendHtml(message);

    if (shouldScroll)
        vert->setValue(vert->maximum());

    if (logFile.isOpen()) {
        logFile.write(message.toUtf8());
        logFile.write("\n");
        logFile.flush();
    }
}

NovenaTestWindow::~NovenaTestWindow()
{
    delete ui;
}
