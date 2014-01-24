#include <fcntl.h>
#include <QString>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef linux
#include <linux/input.h>
#endif

#include "switchtest.h"

SwitchTest::SwitchTest()
{
    name = "Switch test";
}

void SwitchTest::runTest() {
    QString str;
#ifdef linux
    int fd = open("/dev/input/event0", O_RDONLY);
    struct input_event e;

    if (-1 == fd) {
        testError(QString("Unable to open switch: %1").arg(strerror(errno)));
        return;
    }

    testInfo("Please press the side switch");

    if (read(fd, &e, sizeof(e)) != sizeof(e)) {
        testError(QString("Unable to read switch: %1").arg(strerror(errno)));
        close(fd);
        return;
    }

    close(fd);

    testInfo("Side switch OK");
#else
    testInfo("Switch test skipped on this platform");
#endif

    return;
}
