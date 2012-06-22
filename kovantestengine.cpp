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
	errorCount = 0;
}


void KovanTestEngine::setDebug(bool on)
{
	debugMode = on;
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
    return true;
}

/* Returns true if there are more tests to run */
bool KovanTestEngine::runAllTests() {
    currentTestNumber = -1;
    return runNextTest();
}

static const char *levelStr[] = {
    "<font color=\"blue\">INFO</font>",
    "<font color=\"red\">ERROR</font>",
    "<font color=\"green\">DEBUG</font>",
};

void KovanTestEngine::updateTestState(int level, int value, QString *message) {
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
		str.append(tests.at(currentTestNumber)->testName());
		ui->setErrorString(str);
		return false;
	}

	// Increment the test number, and return if we've run out of tests.
    currentTestNumber++;
    if (currentTestNumber >= tests.count()) {
        ui->setProgressBar(1);
        return false;
    }

    currentTest = tests[currentTestNumber];

    QObject::connect(currentTest, SIGNAL(testStateUpdated(int,int,QString*)),
                     this, SLOT(updateTestState(int,int,QString*)));

    currentThread = new KovanTestEngineThread(currentTest);
    QObject::connect(currentThread, SIGNAL(finished()),
                     this, SLOT(cleanupCurrentTest()));
    currentThread->start();

    ui->setStatusText(currentTest->testName());
    ui->setProgressBar(currentTestNumber*1.0/tests.count());
    QString progressText;
    progressText.sprintf("Progress: %d/%d", currentTestNumber, tests.count()-1);
    ui->setProgressText(progressText);
    return true;
}
