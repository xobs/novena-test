#ifndef HDMITEST_H
#define HDMITEST_H
#include <stdint.h>
#include "kovantest.h"

class HDMITest : public KovanTest
{
	Q_OBJECT
public:
    HDMITest();
    void runTest();
    void loadFpgaFirmware(const uint8_t *bytes, ssize_t size);
	void loadDefaultFirmware();
	void loadTestFirmware();
};

#endif // HDMITEST_H
