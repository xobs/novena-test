#include "programsenoko.h"

#include <QProcess>
#include <QFile>
#include <unistd.h>

ProgramSenoko::ProgramSenoko(const QString _firmware) : firmware(_firmware)
{
    name = "Program Senoko";
}

int ProgramSenoko::prepGPIO(void)
{
    QFile exportFile("/sys/class/gpio/export");
    if (!exportFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO export file: " + exportFile.errorString());
        return 1;
    }
    exportFile.write("149");
    exportFile.close();

    QFile directionFile("/sys/class/gpio/gpio149/direction");
    if (!directionFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO149 direction file: " + directionFile.errorString());
        return 1;
    }
    directionFile.write("out\n");
    directionFile.close();

    QFile valueFile("/sys/class/gpio/gpio149/value");
    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO149 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("0\n");
    valueFile.close();

    return 0;
}

int ProgramSenoko::resetGPIO(void)
{
    QFile valueFile("/sys/class/gpio/gpio149/value");

    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO149 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("0\n");
    valueFile.close();

    sleep(1);

    if (!valueFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open GPIO149 value file: " + valueFile.errorString());
        return 1;
    }
    valueFile.write("1\n");
    valueFile.close();

    return 0;
}

void ProgramSenoko::runTest()
{
    QProcess stm32flash;
    int tries;

    if (prepGPIO()) {
        testError("Unable to set up reset GPIO");
        return;
    }

    testInfo("Please hold the \"Reflash\" button on Senoko");
    
    /* Wait for the flashing process to start */
    while (1) {
        bool foundBanner = false;

        /* This string is the last thing to appear before programming starts */
        const char *searchString = "Serial Config: 115200 8E1";

        testDebug("Trying to initiate upload...");
        resetGPIO();

        stm32flash.terminate();

        stm32flash.setProcessChannelMode(QProcess::MergedChannels);
        stm32flash.start("/factory/stm32flash", QStringList()
                           << "-b" << "115200"
                           << "-w" << firmware
                           << "/dev/ttymxc3");
        stm32flash.waitForStarted();

        tries = 0;
        while (!foundBanner && (tries++ < 10)) {
            if (!stm32flash.waitForReadyRead(1000)) {
                testDebug("Process wasn't ready for read in 1 second");
                break;
            }

            QByteArray output = stm32flash.readLine();
            testDebug(QString("Output string: ") + output);

            if (stm32flash.state() != QProcess::Running) {
                testDebug("Process no longer running");
                break;
            }

            if (output.startsWith(searchString)) {
                testDebug("Found search string");
                foundBanner = true;
            }
            else {
                testDebug(QString("Line doesn't contain string %1: %2").arg(searchString).arg((const char *)output));
            }
        }

        if (!foundBanner) {
            testDebug("Didn't find banner");
            continue;
        }

        if (!stm32flash.waitForReadyRead(1000)) {
            testDebug("Didn't find data after 1 second");
            continue;
        }

        break;
    }

    testInfo("Upload started, release the \"Reflash\" button");

    if (!stm32flash.waitForFinished(60000)) {
        testError("Flashing process took too long");
        testError(stm32flash.readAllStandardOutput());
        return;
    }

    testInfo("Senoko reflashed successfully");
    resetGPIO();

    testInfo("Attempting to rescan device bus");
    QFile autoprobeFile("/sys/bus/platform/drivers_autoprobe");

    if (!autoprobeFile.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open platform/drivers_autoprobe file: " + autoprobeFile.errorString());
        return;
    }
    autoprobeFile.write("1\n");
    autoprobeFile.close();

    sleep(1);
}
