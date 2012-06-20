#ifndef HDMITEST_H
#define HDMITEST_H
#include <stdint.h>
#include "kovantest.h"

class HDMITest : public KovanTest
{
public:
    HDMITest();
    void runTest();
    void loadFpgaFirmware(const uint8_t *bytes, ssize_t size);
};

#endif // HDMITEST_H
