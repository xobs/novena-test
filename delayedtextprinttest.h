#ifndef DELAYEDTEXTPRINTTEST_H
#define DELAYEDTEXTPRINTTEST_H
#include "novenatest.h"

class DelayedTextPrintTest : public NovenaTest
{
	Q_OBJECT
public:
    DelayedTextPrintTest(QString message, double delay);
    void runTest();

private:
    const QString message;
    double duration;
};

#endif // DELAYEDTEXTPRINTTEST_H
