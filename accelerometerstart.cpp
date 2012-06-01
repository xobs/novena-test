#include "accelerometerstart.h"

AccelerometerStart::AccelerometerStart()
{
}

void AccelerometerStart::runTest() {
    qDebug("In KovanTestEngine::testStateUpdated()");
    emit testStateUpdated(0, 0, new QString("Did run test"));
    return;
}
