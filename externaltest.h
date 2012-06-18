#ifndef ACCELEROMETERSTART_H
#define ACCELEROMETERSTART_H

#include <stdarg.h>
#include "kovantest.h"

class ExternalTest : public KovanTest
{
    int testNumber;
public:
    ExternalTest(QString *testName);
    void runTest();
    void harnessBridge(int level, int code, char *fmt, va_list ap);
};

#endif // ACCELEROMETERSTART_H
