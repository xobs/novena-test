#include <QString>
#include "externaltest.h"

struct tests {
        int (*func)(void);
        const char *name;
	const char *abbr;
};


extern "C" {
    extern int test_servo(void);
    extern int test_io(void);
    extern int test_usb(void);
    extern int test_audio(void);
    extern int test_serial(void);
    extern int test_accel_start(void);
    extern int test_accel_finish(void);
    extern int test_wifi(void);
}



static struct tests tests[] = {
        { // 0  
                test_accel_start,
                "Begin background accelerometer read",
                "test-accel-start",
        },
        { // 1  
                test_audio,
                "Test audio generation",
                "test-audio",
        },
        { // 2  
                test_serial,
                "Test serial ports",
                "test-serial",
        },
        { // 3  
                test_servo,
                "ADC and servos",
                "test-servo",
        },
        { // 4  
                test_io,
                "Digital I/O",
                "test-io",
        },
        { // 5  
                test_usb,
                "OTG and USB port",
                "test-usb",
        },
        { // 6  
                test_accel_finish,
                "Read accelerometer result",
                "test-accel-finish",
        },
        { // 7  
                test_wifi,
                "Wifi and other USB port",
                "test-wifi",
        },
};


ExternalTest::ExternalTest(QString *testName)
{
    unsigned int i;

    name = new QString("ExternalTest");

    testNumber = -1;
    for (i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        if (testName == QString::fromAscii(tests[i].abbr)) {
            testNumber = i;
            break;
        }
    }
}


void ExternalTest::runTest() {
    qDebug("In ExternalTest::runTest()");
    emit testStateUpdated(0, 0, 0, new QString("Did run test"));
    return;
}
