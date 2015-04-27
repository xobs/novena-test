#include <QDebug>
#include <QScrollBar>
#include <QStyle>
#include <QFile>
#include <QDir>
#include <QProcess>

#include "novenatestwindow.h"
#include "ui_novenatestwindow.h"
#include "waitfornetwork.h"

NovenaTestWindow::NovenaTestWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NovenaTestWindow),
    scrollTimer(parent)
{

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
    showFullScreen();
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

    // Work around a bug where the scroll view doesn't scroll all the way
    connect(&scrollTimer, SIGNAL(timeout()), this, SLOT(scrollView()));

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
    if (!logFile.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Unable to create logfile" << logFile.fileName();
        return;
    }

    logFile.write("<p>Started a new test\n");
    logFile.flush();
    return;
}

void NovenaTestWindow::postLogFile()
{
    system("cp /factory/factory-logs-key /tmp");
    system("chown root:root /tmp/factory-logs-key");
    system("chmod 0600 /tmp/factory-logs-key");
    system(QString("ip li set mtu 492 dev %1").arg(WaitForNetwork::getInterfaceName(WaitForNetwork::WiFi)).toUtf8());
    QProcess sftp;

    sftp.start("sftp", QStringList()
               << "-o" << "UserKnownHostsFile=/dev/null"
               << "-o" << "StrictHostKeyChecking=no"
               << "-i" << "/tmp/factory-logs-key"
               << "factory-logs@bunniefoo.com");

    if (!sftp.waitForStarted()) {
        setStatusText("Unable to start sftp");
        return;
    }

    sftp.write(QString("put %1\nexit\n").arg(logFile.fileName()).toUtf8());

    if (!sftp.waitForFinished()) {
        setStatusText(sftp.readAllStandardError());
        return;
    }

    if (sftp.exitCode()) {
        setStatusText(sftp.readAllStandardError());
        return;
    }
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
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    vert->setValue(vert->maximum());
    scrollTimer.start(300);
}

void NovenaTestWindow::setProgressBar(double progress)
{
    ui->progressBar->setValue(progress*100);
    scrollTimer.start(300);
}

void NovenaTestWindow::setProgressText(QString &message)
{
    ui->progressLabel->setText(message);
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    vert->setValue(vert->maximum());
    scrollTimer.start(300);
}

void NovenaTestWindow::setErrorString(QString &message)
{
    errorString = message;
    ui->errorLabel->setText(errorString);
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    vert->setValue(vert->maximum());
    scrollTimer.start(300);
}

void NovenaTestWindow::scrollView(void)
{
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    vert->setValue(vert->maximum());
}

void NovenaTestWindow::finishTests(bool successful)
{
    QProcess dmesg;
    QProcess ifconfig;

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

    /* Copy dmesg */
    dmesg.start("dmesg");
    dmesg.waitForFinished();
    logFile.write("<p><strong>dmesg output:</strong><p><pre>");
    logFile.write(dmesg.readAllStandardOutput());
    logFile.write("</pre>");

    ifconfig.start("ifconfig -a");
    ifconfig.waitForFinished();
    logFile.write("<p><strong>ifconfig -a:</strong><p><pre>");
    logFile.write(ifconfig.readAllStandardOutput());
    logFile.write("</pre>");

    if (engine->debugModeOn()) {
        ui->testSelectionButton->setVisible(true);
    }

    logFile.close();
    postLogFile();
}

void NovenaTestWindow::addTestLog(QString &message)
{
    QScrollBar *vert = ui->testLog->verticalScrollBar();
    bool shouldScroll = true;

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
