#ifndef ACCELEROMETERSTART_H
#define ACCELEROMETERSTART_H

#include "kovantest.h"

class ExternalTest : public KovanTest
{
    int testNumber;
public:
    ExternalTest(QString *testName);
    void runTest();
};

#endif // ACCELEROMETERSTART_H
