#ifndef COPYMMCTOSATATEST_H
#define COPYMMCTOSATATEST_H
#include "novenatest.h"

class CopyMMCToSataTest : public NovenaTest
{
public:
    CopyMMCToSataTest(QString src, QString dst);
    void runTest();
};

#endif // COPYMMCTOSATATEST_H
