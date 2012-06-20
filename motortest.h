#ifndef MOTORTEST_H
#define MOTORTEST_H
#include <stdint.h>
#include "kovantest.h"

class MotorTest : public KovanTest
{
    uint16_t pwm_divider;
public:
    MotorTest();
    void runTest();
    int dutyFromPercent(int percentage);
    int testMotorNumber(int number);
    int setMotorDirection(int motor, int direction);
    int setDutyCycle(uint16_t cycle);
};

#endif // MOTORTEST_H
