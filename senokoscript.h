#ifndef SENOKOSCRIPT_H
#define SENOKOSCRIPT_H
#include "novenatest.h"

class SenokoScript : public NovenaTest
{
public:
    SenokoScript(const QStringList _commands);
    void runTest();

private:
    int resetGPIO(void);
    QStringList commands;
};

#endif // SENOKOSCRIPT_H
