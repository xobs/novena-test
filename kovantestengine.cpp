#include "kovantestengine.h"
#include "kovantest.h"
#include "kovantestwindow.h"

#include "externaltest.h"
#include "delayedtextprinttest.h"
#include "wifitest.h"
#include "batterytest.h"
#include "motortest.h"
#include "switchtest.h"
#include "hdmitest.h"
#include "fpga.h"

#include <QThread>
#include <QDebug>
class KovanTest;
class KovanTestEngineThread : public QThread {
    KovanTest *tst;
public:
    KovanTestEngineThread(KovanTest *t) {
        tst = t;
    }

    void run() {
        tst->runTest();
    };
    QString *testName() {
        return tst->testName();
    }
};



KovanTestEngine::KovanTestEngine(KovanTestWindow *ui)
{
    currentTest = NULL;
    currentTestNumber = -1;
    currentThread = NULL;
    this->ui = ui;
	debugMode = false;

	unsigned char serialTmp[7];
	read_fpga_serial(serialTmp);
	serialNumberString.sprintf("%02x%02x%02x%02x-%02x%02x-%02x",
							  serialTmp[0], serialTmp[1], serialTmp[2], serialTmp[3],
							  serialTmp[4], serialTmp[5],
							  serialTmp[6]);
}


void KovanTestEngine::setDebug(bool on)
{
	debugMode = on;
}

bool KovanTestEngine::debugModeOn()
{
	return debugMode;
}

const QString &KovanTestEngine::serialNumber() {
	return serialNumberString;
}

bool KovanTestEngine::loadAllTests() {
    tests.append(new DelayedTextPrintTest(new QString("Starting tests..."), 1));
    tests.append(new ExternalTest(new QString("test-accel-start")));
    tests.append(new HDMITest());
    tests.append(new ExternalTest(new QString("test-audio")));
    tests.append(new ExternalTest(new QString("test-serial")));
    tests.append(new ExternalTest(new QString("test-servo")));
    tests.append(new ExternalTest(new QString("test-io")));
    tests.append(new ExternalTest(new QString("test-usb")));
    tests.append(new ExternalTest(new QString("test-accel-finish")));
    tests.append(new WifiTest());
    tests.append(new BatteryTestStart());
    tests.append(new MotorTest());
    tests.append(new BatteryTestStop());
	tests.append(new DelayedTextPrintTest(new QString("Done!"), 0));

	/* Wire up all signals and slots */
	int i;
	for (i=0; i<tests.count(); i++)
		connect(tests.at(i), SIGNAL(testStateUpdated(int,int,QString*)),
				this, SLOT(updateTestState(int,int,QString*)));

    return true;
}

const QList<KovanTest *> & KovanTestEngine::allTests() {
	return tests;
}

/* Returns true if there are more tests to run */
bool KovanTestEngine::runAllTests() {
    currentTestNumber = -1;
	errorCount = 0;
	testsToRun.clear();
	testsToRun = tests;
    return runNextTest();
}

bool KovanTestEngine::runSelectedTests(QList<KovanTest *> &newTests)
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

void KovanTestEngine::updateTestState(int level, int value, QString *message)
{

	QString str;
    str.append("<p>");
    str.append(levelStr[level]);
    str.append(": ");
    str.append(message);
    ui->addTestLog(str);

    if (level == TEST_INFO)
        qDebug() << "INFO:" << level << value << message->toAscii();
	else if (level == TEST_ERROR) {
        qDebug() << "ERROR:" << level << value << message->toAscii();
		errorCount++;
		QString str;
		str.append(testsToRun.at(currentTestNumber)->testName());
		ui->setErrorString(str);
	}
    else if (level == TEST_DEBUG)
        qDebug() << "DEBUG:" << level << value << message->toAscii();
    else
        qDebug() << "????:" << level << value << message->toAscii();

	delete message;
}



void KovanTestEngine::cleanupCurrentTest() {
    delete currentThread;
    currentThread = NULL;
    runNextTest();
    return;
}



bool KovanTestEngine::runNextTest(int continueOnErrors)
{
	if (errorCount && !continueOnErrors && !debugMode) {
		QString str;
		str.append(testsToRun.at(currentTestNumber)->testName());
		ui->finishTests(false);
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

    currentThread = new KovanTestEngineThread(currentTest);
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
