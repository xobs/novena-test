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
#include "acceltest.h"
#include "audiotest.h"
#include "buttontest.h"
#include "capacitytest.h"
#include "delayedtextprinttest.h"
#include "destructivedisktest.h"
#include "eepromtest.h"
#include "fpgatest.h"
#include "gpbbtest.h"
#include "hwclocktest.h"
#include "keyboardmousetest.h"
#include "mmctest.h"
#include "netperftest.h"
#include "packageinstaller.h"
#include "programsenoko.h"
#include "stmpetest.h"
#include "timertest.h"
#include "usbtest.h"
#include "waitfornetwork.h"

#define NOVENA_DESKTOP

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
    quint32 *mem_32 = NULL;
    int mem_range = 0x021b0000;

    _serialNumber = "no-serial";

    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        updateTestState("system", TEST_INFO, 0, "Couldn't open /dev/mem");
        goto out;
    }

    mem_32 = (quint32 *)mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, mem_range&~0xFFFF);
    if (-1 == (int)mem_32) {
        updateTestState("system", TEST_INFO, 0, QString() + "Couldn't mmap /dev/mem: " + strerror(errno));
        goto out;
    }

    hash->addData((const char *)&mem_32[0xc400], 4);
    hash->addData((const char *)&mem_32[0xc410], 4);
    hash->addData((const char *)&mem_32[0xc420], 4);
    hash->addData((const char *)&mem_32[0xc430], 4);
    hash->addData((const char *)&mem_32[0xc440], 4);
    hash->addData((const char *)&mem_32[0xc450], 4);
    hash->addData((const char *)&mem_32[0xc460], 4);
    hash->addData((const char *)&mem_32[0xc470], 4);
    hash->addData((const char *)&mem_32[0xc480], 4);
    hash->addData((const char *)&mem_32[0xc490], 4);
    hash->addData((const char *)&mem_32[0xc4a0], 4);
    hash->addData((const char *)&mem_32[0xc4b0], 4);
    hash->addData((const char *)&mem_32[0xc4c0], 4);
    hash->addData((const char *)&mem_32[0xc4d0], 4);
    hash->addData((const char *)&mem_32[0xc4e0], 4);
    hash->addData((const char *)&mem_32[0xc4f0], 4);

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
    tests.append(new TimerTestStart());
#if defined(NOVENA_BAREBOARD)
    tests.append(new DelayedTextPrintTest(QString("Starting tests..."), 1));
    tests.append(new TimerTestStart());
    tests.append(new STMPETest());
    tests.append(new GPBBTest());
    tests.append(new MMCTestStart("/factory/novena-mmc-disk.img",
                                 MMCCopyThread::getInternalBlockName()));
    tests.append(new KeyboardMouseTest());
    tests.append(new AccelTest());
    tests.append(new WaitForNetwork(WaitForNetwork::WiFi));
    tests.append(new HWClockTestStart());
    tests.append(new EEPROMStart("http://bunniefoo.com:8674/getnew/", "es8328,pcie,gbit,hdmi,eepromoops"));
    tests.append(new NetPerfTest());
    tests.append(new USBTest());
    tests.append(new FpgaTest());
    tests.append(new MMCTestFinish());
    tests.append(new HWClockTestFinish());
    tests.append(new EEPROMFinish("http://bunniefoo.com:8674/assign/by-serial/%1/"));
#elif defined(NOVENA_DESKTOP)
    tests.append(new CapacityTest(CapacityTest::InternalDevice, 12 * 1024 * 1024, -1));
    tests.append(new EEPROMUpdate("es8328,pcie,gbit,hdmi,eepromoops,senoko,edp"));
    tests.append(new DestructiveDiskTest(1024 * 1024 * 32, "/dev/disk/by-path/platform-ci_hdrc.1-usb-0:1.4.3:1.0-scsi-0:0:0:0", "Internal"));
    tests.append(new DestructiveDiskTest(1024 * 1024 * 32, "/dev/disk/by-path/platform-ci_hdrc.1-usb-0:1.4.2:1.0-scsi-0:0:0:0", "External"));
    tests.append(new ProgramSenoko("/factory/senoko.hex"));
    tests.append(new ButtonTest(ButtonTest::PowerButton | ButtonTest::LidSwitch | ButtonTest::CustomButton));
    tests.append(new PackageInstaller("/factory/",
                                      QStringList()
                                      << "xorg-novena_1.3-r2_all.deb"
                                      << "linux-image-novena_3.17-novena-rc3_armhf.deb"
                                      << "linux-headers-novena_3.17-novena-rc3_armhf.deb"
                                      << "u-boot-novena_2014.10-novena-rc14_armhf.deb"));
#else
#error "No board type defined!  Must be: NOVENA_BAREBOARD or NOVENA_DESKTOP"
#endif
    tests.append(new TimerTestStop());
    //tests.append(new AudioTest());
    tests.append(new DelayedTextPrintTest(QString("Done!"), 0));

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
    str.append(" ");
    str.append("<font color=\"#0044aa\">");
    str.append(name);
    str.append("</font>");
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
        emit testsFinished();
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
