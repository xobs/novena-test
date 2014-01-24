#ifndef NETPERFTEST_H
#define NETPERFTEST_H
#include <QProcess>
#include "novenatest.h"

class NetPerfTest : public NovenaTest
{
public:
    NetPerfTest();
    void runTest();

    enum interface {
	    GBit,
	    PCIe,
	    USB,
    };

    static const QString getInterfaceName(enum interface iface);

private:
    int setMTU(const QString &iface);
    int setIPAddress(const QString &iface, const QString &address);
    int pgset(const QString &device, const QString &cmd);
    const QString getMacAddress(const QString &iface);

    int initModule(const QString &modname);
    int getNetworkBytes(const QString &iface, qulonglong &tx, qulonglong &rx);
};

#endif // NETPERFTEST_H
