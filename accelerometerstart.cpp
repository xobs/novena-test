#include "accelerometerstart.h"

AccelerometerStart::AccelerometerStart()
{
    name = new QString("Accelerometer");
}

void AccelerometerStart::runTest() {
    qDebug("In KovanTestEngine::testStateUpdated()");
    emit testStateUpdated(0, 0, new QString("Did run test"));
    return;
}
