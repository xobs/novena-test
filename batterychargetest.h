#ifndef BATTERYCHARGETEST_H
#define BATTERYCHARGETEST_H
#include "novenatest.h"

#include <QThread>

class BatteryChargeMonitor : public QThread {
    Q_OBJECT

public:
    BatteryChargeMonitor(void);
    void run(void);

private:
    bool stop_run;
};

class BatteryChargeTestStart : public NovenaTest
{
public:
    BatteryChargeTestStart(void);
    void runTest();

private:
    BatteryChargeMonitor *chargeMonitor;
};

class BatteryChargeTestCondition : public NovenaTest
{
public:
    BatteryChargeTestCondition(void);
    void runTest();
};

class BatteryChargeTestFinish : public NovenaTest
{
public:
    BatteryChargeTestFinish(void);
    void runTest();
};
#endif // BATTERYCHARGETEST_H
