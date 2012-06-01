#include "kovantestengine.h"
#include "kovantest.h"
#include "kovantestwindow.h"

#include "accelerometerstart.h"
#include "delayedtextprinttest.h"


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
};

KovanTestEngine::KovanTestEngine(KovanTestWindow *ui)
{
    currentTest = NULL;
    currentTestNumber = -1;
    this->ui = ui;
}

bool KovanTestEngine::loadAllTests() {
    tests.append(new DelayedTextPrintTest(new QString("Starting tests..."), 2));
    tests.append(new AccelerometerStart());
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
        runNextTest();
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

    KovanTestEngineThread *newTestThread = new KovanTestEngineThread(currentTest);
    newTestThread->start();

    ui->setProgressBar(currentTestNumber*1.0/tests.count());
    return true;
}
