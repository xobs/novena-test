#ifndef BATTERYTEST_H
#define BATTERYTEST_H
#include "kovantest.h"

class BatteryTestStart : public KovanTest
{
public:
    BatteryTestStart();
    void runTest();
};


class BatteryTestStop : public KovanTest
{
public:
    BatteryTestStop();
    void runTest();
};

#endif // BATTERYTEST_H
