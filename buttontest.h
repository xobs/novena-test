#ifndef BUTTONTEST_H
#define BUTTONTEST_H
#include "novenatest.h"

class ButtonTest : public NovenaTest
{
public:
    enum button {
        PowerButton = (1 << 0),
        CustomButton = (1 << 1),
        LidSwitch = (1 << 2),
    };

    ButtonTest(enum button _buttonMask);
    void runTest();

private:
    int buttonMask;
};

#endif // BUTTONTEST_H
