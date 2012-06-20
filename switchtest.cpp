#include <fcntl.h>
#include <QString>
#include <string.h>
#include <errno.h>

#ifdef linux
#include <linux/input.h>
#endif

#include "switchtest.h"

SwitchTest::SwitchTest()
{
    name = new QString("Switch test");
}

void SwitchTest::runTest() {
    QString *str;
#ifdef linux
    int fd = open("/dev/input/event0", O_RDONLY);
    struct input_event e;

    if (-1 == fd) {
        str = new QString("Unable to open switch: ");
        str->append(strerror(errno));
        emit testStateUpdated(TEST_ERROR, 0, str);
        return;
    }

    str = new QString("Please press the side switch");
    emit testStateUpdated(TEST_INFO, 0, str);

    if (read(fd, &e, sizeof(e)) != sizeof(e)) {
        str = new QString("Unable to read switch: ");
        str->append(strerror(errno));
        emit testStateUpdated(TEST_ERROR, 0, str);
        close(fd);
        return;
    }

    close(fd);


    str = new QString("Side switch OK");
    emit testStateUpdated(TEST_INFO, 0, str);
#else
    str = new QString("Switch test skipped on this platform");
    emit testStateUpdated(TEST_INFO, 0, str);
#endif

    return;
}
