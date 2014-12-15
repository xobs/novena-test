#include <QThread>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QTime>
#include "timertest.h"

static QTime *timer;

TimerTestStart::TimerTestStart()
{
    name = "Timer test start";
    timer = new QTime();
}

void TimerTestStart::runTest()
{
    testInfo("Starting stopwatch");
    timer->start();
}

TimerTestStop::TimerTestStop()
{
    name = "Timer test finish";
}

void TimerTestStop::runTest()
{
    testDebug(QString("Test took %1 ms").arg(timer->elapsed()));
    testInfo("Timer finished");
}
