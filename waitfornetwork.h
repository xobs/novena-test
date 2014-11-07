#ifndef WAITFORNETWORK_H
#define WAITFORNETWORK_H
#include "novenatest.h"

class WaitForNetwork : public NovenaTest
{

public:

    enum interface {
        GBit,
        WiFi,
        USB,
    };

    WaitForNetwork(enum interface iface);
    void runTest();
    static const QString getInterfaceName(enum interface iface);

private:
    enum interface _iface;
};

#endif // WAITFORNETWORK_H
