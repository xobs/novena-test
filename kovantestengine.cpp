#include "kovantestengine.h"
#include "kovantest.h"
#include "kovantestwindow.h"

#include "externaltest.h"
#include "delayedtextprinttest.h"
#include "wifitest.h"

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
}



bool KovanTestEngine::loadAllTests() {
    tests.append(new DelayedTextPrintTest(new QString("Starting tests..."), 1));
    tests.append(new WifiTest());
    tests.append(new ExternalTest(new QString("test-accel-start")));
    tests.append(new ExternalTest(new QString("test-audio")));
    tests.append(new ExternalTest(new QString("test-serial")));
    tests.append(new ExternalTest(new QString("test-servo")));
    tests.append(new ExternalTest(new QString("test-io")));
    tests.append(new ExternalTest(new QString("test-usb")));
    tests.append(new ExternalTest(new QString("test-accel-finish")));
    tests.append(new DelayedTextPrintTest(new QString("Stopping tests..."), 1));
    tests.append(new DelayedTextPrintTest(new QString("Done!"), 0));
    return true;
}

/* Returns true if there are more tests to run */
bool KovanTestEngine::runAllTests() {
    currentTestNumber = -1;
    return runNextTest();
}


void KovanTestEngine::updateTestState(int running, int level, int value, QString *message) {
    ui->setStatusText(message);
}



void KovanTestEngine::cleanupCurrentTest() {
    delete currentThread;
    currentThread = NULL;
    runNextTest();
    return;
}

bool KovanTestEngine::runNextTest()
{
    currentTestNumber++;
    if (currentTestNumber >= tests.count()) {
        ui->setProgressBar(1);
        return false;
    }

    currentTest = tests[currentTestNumber];

    QObject::connect(currentTest, SIGNAL(testStateUpdated(int,int,int,QString*)),
                     this, SLOT(updateTestState(int,int,int,QString*)));

    currentThread = new KovanTestEngineThread(currentTest);
    QObject::connect(currentThread, SIGNAL(finished()),
                     this, SLOT(cleanupCurrentTest()));
    currentThread->start();

    ui->setProgressBar(currentTestNumber*1.0/tests.count());
    return true;
}
