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

    WaitForNetwork(enum interface iface, bool testExists = false);
    void runTest();
    static const QString getInterfaceName(enum interface iface);

private:
    enum interface _iface;
    bool earlyExit;
};

#endif // WAITFORNETWORK_H
