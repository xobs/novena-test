#ifndef HWCLOCKTEST_H
#define HWCLOCKTEST_H
#include "novenatest.h"


class HWClockTestStart : public NovenaTest
{
private:
    int getNtpDate(void);
    int sysToRTC(void);

public:
    HWClockTestStart();
    void runTest();
};

class HWClockTestFinish : public NovenaTest
{
public:
    HWClockTestFinish();
    void runTest();
};


#endif // HWCLOCKTEST_H
