Novena Test
===========

This is the testing framework for the Novena laptop.  It is originally
based on the Kovan testing framework.

Tests are arranged as a series of subclasses of the NovenaTest class.  Each
test must implement the *void runTest(void)* method, and should emit a
testStateUpdated(int level, int value, QString \*message) signal every time
the test state is updated.

Creating new tests
------------------

A minimal test would look like:

     #include "novenatest.h"
     class SimpleTest : public NovenaTest
     {
         Q_OBJECT
         public:
             SimpleTest() { }
             void runTest() {
                 emit testStateUpdated(TEST_INFO, 0, new QString("Test completed"));
             }
    };

You would then add this test to novenatestengine.cpp in the loadAllTests()
method by adding the following:

    tests.append(new SimpleTest());

When your runTest thread exits, if it hasn't emitted a TEST_ERROR state, then
the test is assumed to have completed successfully.


Current tests
-------------

The following tests have been defined.  Note that tests in italics have not yet been written:

* _Power logger_ (Logs current draw from Senoko)
* SATA prober (Ensures a SATA drive is attached)
* MMC checker (Ensures internal MMC is in place)
* Firmware loader (Loads firmware onto internal MMC, in the background)
* HDMI tester (Puts patterns on the screen)
* USB tester (Loopback?)
* PCIe tester (enumeration?  Ethernet loopback?)
* EEPROM tester (Verify EEPROM exists, update it if necessary)
* String printer (Useful for printing status messages)
