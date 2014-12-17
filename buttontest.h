#ifndef BUTTONTEST_H
#define BUTTONTEST_H
#include "novenatest.h"

class QFile;

class ButtonTest : public NovenaTest
{
public:
    enum button {
        PowerButton = (1 << 0),
        CustomButton = (1 << 1),
        LidSwitch = (1 << 2),
        ACPlug = (1 << 3),
    };

    ButtonTest(int _buttonMask);
    void runTest();

private:
    QFile *openByName(const QString &name);
    int getSenokoRegister(int reg);
    int senoko_fd;
    int buttonMask;
};

#endif // BUTTONTEST_H
