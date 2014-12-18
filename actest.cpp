#include "buttontest.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <errno.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <ctype.h>

#define SENOKO_I2C_FILE "/dev/i2c-0"
#define SENOKO_I2C_ADDR 0x20

QFile *ButtonTest::openByName(const QString &name)
{
    char devName[256] = "???";
    QDir inputDir("/dev/input");

    inputDir.setNameFilters(QStringList() << "event*");
    inputDir.setFilter(QDir::Files | QDir::Readable | QDir::System);
    foreach (QFileInfo fileInfo, inputDir.entryInfoList()) {
        QFile *inputFile = new QFile(fileInfo.absoluteFilePath());
        if (!inputFile->open(QIODevice::ReadOnly)) {
            testDebug(QString("Couldn't open %1: %2").arg(inputFile->fileName()).arg(inputFile->errorString()));
            delete inputFile;
            continue;
        }

        ioctl(inputFile->handle(), EVIOCGNAME(sizeof(devName)), devName);
        if (name == devName) {
            testDebug(QString("Found %1").arg((const char*)devName));
            int flags = fcntl(inputFile->handle(), F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(inputFile->handle(), F_SETFL, flags);
            return inputFile;
        }
        inputFile->close();
        delete inputFile;
    }

    testDebug(QString("No files matched ") + name);
    return NULL;
}

int ButtonTest::getSenokoRegister(int reg)
{
    struct i2c_rdwr_ioctl_data session;
    struct i2c_msg messages[2];
    char set_addr_buf[1];
    qint8 data[1];

    if (senoko_fd <= 0)
        senoko_fd = open(SENOKO_I2C_FILE, O_RDWR);

    memset(set_addr_buf, 0, sizeof(set_addr_buf));
    memset(data, 0, sizeof(data));

    set_addr_buf[0] = reg;

    messages[0].addr = SENOKO_I2C_ADDR;
    messages[0].flags = 0;
    messages[0].len = sizeof(set_addr_buf);
    messages[0].buf = set_addr_buf;

    messages[1].addr = SENOKO_I2C_ADDR;
    messages[1].flags = I2C_M_RD;
    messages[1].len = sizeof(data);
    messages[1].buf = (char *)data;

    session.msgs = messages;
    session.nmsgs = 2;

    if(ioctl(senoko_fd, I2C_RDWR, &session) < 0)
        return -1;

    return ((int)data[0] & 0xff);
}

ButtonTest::ButtonTest(int _buttonMask)
{
    name = "Button Test";
    buttonMask = _buttonMask;
    senoko_fd = 0;
}

void ButtonTest::runTest()
{
    QFile *powerButton = NULL;
    QFile *lidSwitch = NULL;
    QFile *customButton = NULL;
    QStringList buttonNames;
    struct input_event evt;

    if (buttonMask & PowerButton) {
        powerButton = openByName("Senoko keypad");
        if (!powerButton) {
            testInfo("Couldn't find Senoko keypad, will poll");
        }
        buttonNames.append("Power button");
    }

    if (buttonMask & ACPlug) {
        buttonNames.append("AC plug");
    }

    if (buttonMask & LidSwitch) {
        lidSwitch = openByName("gpio-keys");
        if (!lidSwitch) {
            testError("Couldn't find gpio-keys");
            return;
        }
        buttonNames.append("Lid switch");
    }

    if (buttonMask & CustomButton) {
        customButton = openByName("20b8000.kpp");
        if (!customButton) {
            testError("Couldn't find 20b8000.kpp");
            return;
        }
        buttonNames.append("User button");
    }

    testInfo(QString("Please trigger these buttons: ").append(buttonNames.join(",")));

    int loops = 0;
    while (buttonMask) {
        loops++;

        if ((buttonMask & PowerButton) && powerButton) {
            memset((void *)&evt, 0, sizeof(evt));
            int ret = read(powerButton->handle(), (char *)&evt, sizeof(evt));
            if (ret == sizeof(evt)) {
                if ((evt.type == EV_KEY) && (evt.code == KEY_POWER)) {
                    testInfo("Got Power key (evdev)");
                    buttonMask &= ~PowerButton;
                }
                else if ((evt.type == EV_MSC) && (evt.code == MSC_SCAN))
                    testDebug("Got keypad scan (custom key should be coming up soon)");
                else
                    testInfo(QString("Got unknown key from Senoko: Type: %1  Code: %2").arg(QString::number(evt.type, 16)).arg(QString::number(evt.code, 16)));
            }
        }

        if ((buttonMask & PowerButton)) {
            int ret = getSenokoRegister(0x0f);
            if (ret >= 0 && (ret & (1 << 4))) {
                testInfo("Got Power key (i2c direct)");
                buttonMask &= ~PowerButton;
            }
        }

        if (buttonMask & ACPlug) {
            int ret = getSenokoRegister(0x0f);
            if (ret >= 0 && !(ret & (1 << 3))) {
                testInfo("Got AC unplug (i2c direct)");
                buttonMask &= ~ACPlug;
            }
        }

        if ((buttonMask & LidSwitch) && lidSwitch) {
            memset((void *)&evt, 0, sizeof(evt));
            int ret = read(lidSwitch->handle(), (char *)&evt, sizeof(evt));
            if (ret == sizeof(evt)) {
                if ((evt.type == EV_SW) && (evt.code == SW_LID)) {
                    testInfo("Got Lid switch");
                    buttonMask &= ~LidSwitch;
                }
                else
                    testInfo(QString("Got unknown key from GPIO: Type: %1  Code: %2").arg(QString::number(evt.type, 16)).arg(QString::number(evt.code, 16)));
            }
        }

        if ((buttonMask & CustomButton) && customButton) {
            memset((void *)&evt, 0, sizeof(evt));
            int ret = read(customButton->handle(), (char *)&evt, sizeof(evt));
            if (ret == sizeof(evt)) {
                if ((evt.type == EV_KEY) && (evt.code == KEY_CONFIG)) {
                    testInfo("Got Custom key");
                    buttonMask &= ~CustomButton;
                }
                else
                    testInfo(QString("Got unknown key from KPP: Type: %1  Code: %2").arg(QString::number(evt.type, 16)).arg(QString::number(evt.code, 16)));
            }
        }
    }
    testInfo("Got all buttons");
}
