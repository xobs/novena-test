#include "delayedtextprinttest.h"
#include <QString>
#include <QThread>

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


DelayedTextPrintTest::DelayedTextPrintTest(QString msg, double delay)
    : message(msg)
{
    this->duration = delay;
    name = new QString("Text delay");
}

void DelayedTextPrintTest::runTest() {
    emit testStateUpdated(0, 0, message);
    SleeperThread::msleep(duration * 1000);
}
