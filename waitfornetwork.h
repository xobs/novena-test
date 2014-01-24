#ifndef WAITFORNETWORK_H
#define WAITFORNETWORK_H
#include "novenatest.h"

class WaitForNetwork : public NovenaTest
{
public:
    WaitForNetwork();
    void runTest();

    enum interface {
        GBit,
        PCIe,
        USB,
    };

    static const QString getInterfaceName(enum interface iface);
};

#endif // WAITFORNETWORK_H
