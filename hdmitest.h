#ifndef HDMITEST_H
#define HDMITEST_H
#include <stdint.h>
#include "novenatest.h"

class HDMITest : public NovenaTest
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
