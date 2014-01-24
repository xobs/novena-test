#include <QThread>
#include <QDebug>

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
	serialRead = false;
	updateSerialNumber();
}

void NovenaTestEngine::updateSerialNumber()
{
	if (!serialRead) {
		unsigned char serialTmp[7];
//		if (read_fpga_serial(serialTmp))
			return;
            /*
		serialNumberString.sprintf("%02x-%02x%02x%02x-%02x%02x-%02x",
								  serialTmp[0], serialTmp[1], serialTmp[2], serialTmp[3],
								  serialTmp[4], serialTmp[5],
								  serialTmp[6]);
                                  */
		serialRead = true;
	}
}

void NovenaTestEngine::setDebug(bool on)
{
	debugMode = on;
}

bool NovenaTestEngine::debugModeOn()
{
	return debugMode;
}

const QString &NovenaTestEngine::serialNumber() {
	updateSerialNumber();
	return serialNumberString;
}

bool NovenaTestEngine::loadAllTests() {
    tests.append(new DelayedTextPrintTest(QString("Starting tests..."), 1));
    tests.append(new MMCTestStart("/factory/quantum-biosys.tar.gz",
                                  "/factory/u-boot-internal.imx",
                                  MMCCopyThread::getInternalBlockName()));
    tests.append(new WaitForNetwork());
    tests.append(new EEPROMStart("http://192.168.100.11:8000/getnew/", "es8328,pcie,gbit,hdmi"));
    tests.append(new USBTest());
    tests.append(new FpgaTest());
    //tests.append(new NetPerfTest());
    tests.append(new MMCTestFinish());
    tests.append(new EEPROMFinish("http://192.168.100.11:8000/assign/by-serial/%1/"));
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
