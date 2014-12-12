#ifndef CAPACITYTEST_H
#define CAPACITYTEST_H
#include "novenatest.h"

class CapacityTest : public NovenaTest
{
public:
    enum device {
        InternalDevice,
        ExternalDevice,
    };
    CapacityTest(enum device _device, int _min_size_kb, int _max_size_kb);
    void runTest();

private:
    enum device device;
    qlonglong max_size;
    qlonglong min_size;
};

#endif // CAPACITYTEST_H
