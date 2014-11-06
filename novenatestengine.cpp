#include <QThread>
#include <QDebug>
#include <QCryptographicHash>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "novenatestengine.h"
#include "novenatest.h"
#include "novenatestwindow.h"

/* Available Tests */
#include "delayedtextprinttest.h"
#include "mmctest.h"
#include "usbtest.h"
#include "fpgatest.h"
#include "netperftest.h"
#include "eepromtest.h"
#include "waitfornetwork.h"
#include "playmp3.h"

class NovenaTest;
class NovenaTestEngineThread : public QThread {
    NovenaTest *tst;
public:
    NovenaTestEngineThread(NovenaTest *t) {
        tst = t;
    }

    void run() {
        tst->runTest();
    };
    const QString testName() {
        return tst->testName();
    }
};

NovenaTestEngine::NovenaTestEngine(NovenaTestWindow *ui)
{
    currentTest = NULL;
    currentTestNumber = -1;
    currentThread = NULL;
    this->ui = ui;
    debugMode = false;

    getSerial();
}

const QString &NovenaTestEngine::serialNumber(void)
{
    return _serialNumber;
}

void NovenaTestEngine::setDebug(bool on)
{
    debugMode = on;
}

bool NovenaTestEngine::debugModeOn()
{
    return debugMode;
}

void NovenaTestEngine::getSerial(void)
{
    QCryptographicHash *hash = new QCryptographicHash(QCryptographicHash::Sha1);
    int mem_fd = 0;
    qint32 *mem_32 = NULL;
    int mem_range = 0x021bc000;
    qint32 keys[4];

    _serialNumber = "no-serial";

    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        updateTestState("system", TEST_INFO, 0, "Couldn't open /dev/mem");
        goto out;
    }

    mem_32 = (qint32 *)mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, mem_range&~0xFFFF);
    if (-1 == (int)mem_32) {
        updateTestState("system", TEST_INFO, 0, QString() + "Couldn't mmap /dev/mem: " + strerror(errno));
        goto out;
    }

    keys[0] = mem_32[0x410 / 4];
    keys[1] = mem_32[0x420 / 4];
    keys[2] = mem_32[0x430 / 4];
    keys[3] = mem_32[0x440 / 4];

    hash->addData((const char *)keys, sizeof(keys));

    _serialNumber = hash->result().toHex();
    delete hash;

    updateTestState("system", TEST_INFO, 0, QString() + "Serial number: " + _serialNumber);

out:
    if (mem_fd)
        close(mem_fd);
    if (mem_32)
        munmap(mem_32, 0xFFFF);
}

bool NovenaTestEngine::loadAllTests() {
    tests.append(new DelayedTextPrintTest(QString("Starting tests..."), 1));
//    tests.append(new MMCTestStart("/factory/quantum-biosys.tar.gz",
//                                  "/factory/u-boot-internal.imx",
//                                  MMCCopyThread::getInternalBlockName()));
    tests.append(new WaitForNetwork());
//    tests.append(new EEPROMStart("http://bunniefoo.com:8675/getnew/", "es8328,pcie,gbit,hdmi"));
    tests.append(new USBTest());
//    tests.append(new FpgaTest());
//    tests.append(new MMCTestFinish());
//    tests.append(new EEPROMFinish("http://bunniefoo.com:8675/assign/by-serial/%1/"));
    tests.append(new DelayedTextPrintTest(QString("Done!"), 0));
    tests.append(new PlayMP3("/factory/test_done_nyan.mp3"));

    /* Wire up all signals and slots */
    int i;
    for (i=0; i<tests.count(); i++)
        connect(tests.at(i), SIGNAL(testStateUpdated(QString,int,int,QString)),
                this, SLOT(updateTestState(QString,int,int,QString)));

    return true;
}

const QList<NovenaTest *> & NovenaTestEngine::allTests() {
    return tests;
}

/* Returns true if there are more tests to run */
bool NovenaTestEngine::runAllTests() {
    currentTestNumber = -1;
    errorCount = 0;
    testsToRun.clear();
    testsToRun = tests;
    return runNextTest();
}

bool NovenaTestEngine::runSelectedTests(QList<NovenaTest *> &newTests)
{
    currentTestNumber = -1;
    errorCount = 0;
    testsToRun.clear();
    testsToRun = newTests;
    return runNextTest();
}

static const char *levelStr[] = {
    "<font color=\"blue\">INFO</font>",
    "<font color=\"red\">ERROR</font>",
    "<font color=\"green\">DEBUG</font>",
};

void NovenaTestEngine::updateTestState(const QString name, int level, int value, const QString message)
{

    QString str;
    str.append("<p>");
    str.append(levelStr[level]);
    str.append(": ");
    str.append(message);
    ui->addTestLog(str);

    if (level == TEST_INFO)
        qDebug() << name << "INFO:" << value << message;
    else if (level == TEST_ERROR) {
        qDebug() << name << "ERROR:" << value << message;
        errorCount++;
        QString str;
        str.append(testsToRun.at(currentTestNumber)->testName());
        ui->setErrorString(str);
    }
    else if (level == TEST_DEBUG)
        qDebug() << name << "DEBUG:" << value << message;
    else
        qDebug() << name << "????:" << level << value << message;
}



void NovenaTestEngine::cleanupCurrentTest() {
    delete currentThread;
    currentThread = NULL;
    runNextTest();
    return;
}



bool NovenaTestEngine::runNextTest(int continueOnErrors)
{
    if (errorCount && !continueOnErrors && !debugMode) {
        QString str;
        str.append(testsToRun.at(currentTestNumber)->testName());
        ui->finishTests(false);
        emit testsFinished();
        return false;
    }

    // Increment the test number, and return if we've run out of tests.
    currentTestNumber++;
    if (currentTestNumber >= testsToRun.count()) {
        ui->setProgressBar(1);
        ui->finishTests(errorCount?false:true);
        return false;
    }

    currentTest = testsToRun[currentTestNumber];

    currentThread = new NovenaTestEngineThread(currentTest);
    QObject::connect(currentThread, SIGNAL(finished()),
                     this, SLOT(cleanupCurrentTest()));
    currentThread->start();

    ui->setStatusText(currentTest->testName());
    ui->setProgressBar(currentTestNumber*1.0/testsToRun.count());
    QString progressText;
    progressText.sprintf("Progress: %d/%d", currentTestNumber+1, testsToRun.count());
    ui->setProgressText(progressText);
    return true;
}
