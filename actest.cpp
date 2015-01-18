#include "actest.h"
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

#define MAX_TRIES 10

static int senoko_fd;

static int getSenokoRegisterReal(int reg)
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

static int getSenokoRegister(int reg)
{
    int tries;
    bool success;
    int ret = 0;

    success = false;
    for (tries = 0; !success && (tries < MAX_TRIES); tries++) {
        ret = getSenokoRegisterReal(reg);
        if (ret < 0)
            sleep(1);
        else
            success = true;
    }
    if (!success)
        return -1;
    return ret;
}

ACTest::ACTest(enum direction _dir)
{
    if (_dir == PlugIn) {
        name = "AC plug in test";
        dir = PlugIn;
    }
    else {
        name = "AC unplug test";
        dir = UnPlug;
    }
    senoko_fd = 0;
}

void ACTest::runTest()
{
    int tries = 0;
    if (dir == PlugIn)
        testInfo("Waiting for AC to be connected");
    else
        testInfo("Waiting for AC to be unplugged");

    while (1) {

        int ret = getSenokoRegister(0x0f);
        if (ret < 0)
            continue;

        if ((dir == PlugIn) && (ret & (1 << 3))) {
            testInfo("AC connected");
            break;
        }

        else if ((dir == UnPlug) && !(ret & (1 << 3))) {
            testInfo("AC disconnected");
            break;
        }
        tries++;
        sleep(1);
    }
    testDebug(QString("Test took %1 tries").arg(QString::number(tries)));
}
