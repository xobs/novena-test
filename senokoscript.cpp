#include "senokoscript.h"

#define SERIAL_DEVICE "/dev/ttymxc3"
#define PROMPT "ch> "
#include <QFile>

#include <termios.h>

SenokoScript::SenokoScript(const QStringList _commands) :
    commands(_commands), serial(SERIAL_DEVICE)
{
    name = "Senoko Script";
}

const QString SenokoScript::runCommand(const QString command)
{
    quint8 buf[strlen(PROMPT)];
    int tst;
    int offset = 0;
    int i;
    const char *banner = PROMPT;
    QString output;

    serial.write(command.toUtf8());
    serial.write("\r\n");
    serial.flush();

    while (1) {
        int ret = serial.read((char *)&buf[offset], 1);
        if (ret == 1) {
            tst = (offset + 1) % sizeof(buf);
            output.append(buf[offset]);

            i = 0;
            while (tst != offset) {
                if (banner[i] != buf[tst])
                        break;
                tst++;
                i++;
                tst %= sizeof(buf);
            }
            if ((tst == offset) && (banner[i] == buf[tst])) {
                testDebug(QString("String from Senoko: ").append(output));
                return output;
            }

            offset++;
            offset %= sizeof(buf);
        }
        else if (ret == -1) {
            testError(QString("Unable to read from serial port: %1").arg(serial.errorString()));
            return "";
        }
        else if (ret == 0) {
            testInfo("read() returned 0");
            if (!serial.isOpen()) {
                testError("Serial is closed now?!");
                testError(serial.errorString());
                testError(output);
                return "";
            }
        }
    }

    return "";
}

void SenokoScript::runTest()
{
    int ret;
    struct termios t;

    if (commands.count() & 1) {
        testError("Programmer Error: Tests must be comprised of [command] [result].  Missing at least one [result].");
    }

    testDebug("Opening serial port");
    serial.open(QIODevice::ReadWrite | QIODevice::Unbuffered);
    if (!serial.isOpen()) {
        testError(QString("Unable to open serial device: ").append(serial.errorString()));
        return;
    }

    testDebug("Setting serial port speed");
    ret = tcgetattr(serial.handle(), &t);
    if (-1 == ret) {
        testError("Failed to get serial port attributes");
        return;
    }
    cfsetispeed(&t, B115200);
    cfsetospeed(&t, B115200);
    cfmakeraw(&t);
    ret = tcsetattr(serial.handle(), TCSANOW, &t);
    if (-1 == ret) {
        testError("Failed to set serial port speed");
        return;
    }
    tcdrain(serial.handle());

    testDebug("Looking for Senoko...");
    testInfo(QString("Found Senoko: ").append(runCommand(" ")));

    for (int i = 0; i < commands.count(); i += 2) {
        const QString cmd = commands.at(i);
        const QString needle = commands.at(i + 1);

        testInfo(QString("Running command: ").append(cmd));

        const QString haystack = runCommand(cmd);
        if (!haystack.contains(needle)) {
            testError(QString("Error: String %1 not found in output '%2'").arg(needle).arg(haystack));
            return;
        }
    }
}
