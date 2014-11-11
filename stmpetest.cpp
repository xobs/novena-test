#include <QFile>
#include "stmpetest.h"

#define ACCEL_PATH "/sys/bus/i2c/devices/0-0044/"

STMPETest::STMPETest()
{
    name = "STMPE Test";
}

void STMPETest::runTest()
{
    QFile nameFile(ACCEL_PATH "name");

    testInfo("Reading STMPE name");

    if (!nameFile.open(QIODevice::ReadOnly)) {
        testError("Unable to open device name");
        return;
    }

    /* We must trim whitespace, since toInt() will fail otherwise, and the value
     * contains a trailing \n character.
     */
    QString name = nameFile.readAll().trimmed();

    if (name != "stmpe811")
        testError(QString("Error: Expected name of 'stmpe811', got name of '%1'").arg(name));
    else
        testInfo(QString("Got expected name of '%1'").arg(name));
}
