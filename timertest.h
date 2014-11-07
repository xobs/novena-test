#ifndef TIMERTEST_H
#define TIMERTEST_H
#include <QThread>
#include <QFile>
#include "novenatest.h"

class TimerTestStart : public NovenaTest
{
public:
    TimerTestStart();
    void runTest();
};

class TimerTestStop : public NovenaTest
{
public:
    TimerTestStop();
    void runTest();
};


#endif // TIMERTEST_H
