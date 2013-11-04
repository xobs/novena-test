#include "novenatest.h"

NovenaTest::NovenaTest()
{
    testNumber = -1;
    lastString = NULL;
    lastResult = 0;
    engine = NULL;
    name = new QString("Base test");
}

QString *NovenaTest::getStatusString()
{
    return lastString;
}

int NovenaTest::getStatusValue()
{
    return lastResult;
}

void NovenaTest::setTestNumber(int number)
{
    testNumber = number;
}

void NovenaTest::setEngine(NovenaTestEngine *engie)
{
    engine = engie;
}

QString *NovenaTest::testName()
{
    return name;
}

void NovenaTest::testInfo(const QString string)
{
    emit testStateUpdated(TEST_INFO, 0, string);
}

void NovenaTest::testError(const QString string)
{
    emit testStateUpdated(TEST_ERROR, 0, string);
}
