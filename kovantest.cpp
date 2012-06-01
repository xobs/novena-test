#include "kovantest.h"

KovanTest::KovanTest()
{
    testNumber = -1;
    lastString = NULL;
    lastResult = 0;
    engine = NULL;
    name = new QString("Base test");
}

QString *KovanTest::getStatusString()
{
    return lastString;
}

int KovanTest::getStatusValue() {
    return lastResult;
}

void KovanTest::setTestNumber(int number) {
    testNumber = number;
}

void KovanTest::setEngine(KovanTestEngine *engie) {
    engine = engie;
}

QString *KovanTest::testName() {
    return name;
}
