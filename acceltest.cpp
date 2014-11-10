#include "acceltest.h"

#include <QFile>

#include <QDebug>
#define ACCEL_PATH "/sys/bus/iio/devices/iio:device0/"

AccelTest::AccelTest()
{
    name = "Accelerometer Test";
}

void AccelTest::runTest()
{
    QFile xraw(ACCEL_PATH "in_accel_x_raw");
    QFile yraw(ACCEL_PATH "in_accel_y_raw");
    QFile zraw(ACCEL_PATH "in_accel_z_raw");
    int x, y, z;

    testInfo("Reading accelerometer values");

    if (!xraw.open(QIODevice::ReadOnly)) {
        testError("Unable to open accelerometer X axis");
        return;
    }

    if (!yraw.open(QIODevice::ReadOnly)) {
        testError("Unable to open accelerometer Y axis");
        return;
    }

    if (!zraw.open(QIODevice::ReadOnly)) {
        testError("Unable to open accelerometer Z axis");
        return;
    }

    /* We must trim whitespace, since toInt() will fail otherwise, and the value
     * contains a trailing \n character.
     */
    x = xraw.readAll().trimmed().toInt();
    y = yraw.readAll().trimmed().toInt();
    z = zraw.readAll().trimmed().toInt();

    testInfo(QString("Accelerometer responding.  Values  X: %1  Y: %2  Z: %3").arg(x).arg(y).arg(z));
}
