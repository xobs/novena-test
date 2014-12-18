#ifndef BATTERYCHARGETEST_H
#define BATTERYCHARGETEST_H
#include "novenatest.h"

class QFile;

class BatteryChargeTestStart : public NovenaTest
{
public:
    BatteryChargeTestStart(void);
    void runTest();
};

class BatteryChargeTestFinish : public NovenaTest
{
public:
    BatteryChargeTestFinish(void);
    void runTest();
};
#endif // BATTERYCHARGETEST_H
