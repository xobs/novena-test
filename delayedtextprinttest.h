#ifndef DELAYEDTEXTPRINTTEST_H
#define DELAYEDTEXTPRINTTEST_H
#include "kovantest.h"

class DelayedTextPrintTest : public KovanTest
{
	Q_OBJECT
public:
    DelayedTextPrintTest(QString *message, double delay);
    void runTest();

private:
    QString *message;
    double duration;
};

#endif // DELAYEDTEXTPRINTTEST_H
