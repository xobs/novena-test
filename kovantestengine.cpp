#include "kovantestengine.h"
#include "kovantest.h"
#include "kovantestwindow.h"

#include "accelerometerstart.h"
#include "delayedtextprinttest.h"
#include "audiotest.h"

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
    tests.append(new AccelerometerStart());
    tests.append(new AudioTest());
    tests.append(new DelayedTextPrintTest(new QString("Stopping tests..."), 1));
    tests.append(new DelayedTextPrintTest(new QString("Done!"), 0));
    return true;
}

/* Returns true if there are more tests to run */
bool KovanTestEngine::runAllTests() {
    currentTestNumber = -1;
    return runNextTest();
}

void KovanTestEngine::updateTestState(int running, int value, QString *message) {
    qDebug() << "In KovanTestEngine::updateTestState(" << running << ", " << value << ", " << message->toUtf8() << ")";

    ui->setStatusText(message);

    if (!running)
        currentThread->terminate();
}

void KovanTestEngine::cleanupCurrentTest() {
    if (currentThread)
        qDebug() << "Finishing up test: " << currentThread->testName()->toUtf8();

    if (currentThread && currentThread->isRunning()) {
        currentThread->terminate();
        qDebug() << "Thread was running, telling it to terminate";
        return;
    }
    qDebug() << "Thread was not running.  Moving on to next test.";
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

    QObject::connect(currentTest, SIGNAL(testStateUpdated(int,int,QString*)),
                     this, SLOT(updateTestState(int,int,QString*)));

    currentThread = new KovanTestEngineThread(currentTest);
    QObject::connect(currentThread, SIGNAL(finished()),
                     this, SLOT(cleanupCurrentTest()));
    currentThread->start();

    ui->setProgressBar(currentTestNumber*1.0/tests.count());
    return true;
}
