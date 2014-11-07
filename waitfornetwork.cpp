#include "waitfornetwork.h"
#include <QFileInfoList>
#include <QDir>
#include <QNetworkInterface>
#include <QThread>
#include <QProcess>
#include <QList>
#include <QNetworkAddressEntry>

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


const QString WaitForNetwork::getInterfaceName(enum interface iface)
{
    QString path;
    if (iface == GBit)
        path = "/sys/bus/platform/devices/2188000.ethernet/net/";
    else if (iface == USB)
        path = "/sys/bus/usb/devices/1-1.2:1.0/net/";
    else if (iface == WiFi)
        path = "/sys/devices/soc0/soc/1ffc000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0/net/";
    else
        return QString();

    QDir eth(path);
    QFileInfoList list = eth.entryInfoList();

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (!fileInfo.fileName().startsWith("."))
            return fileInfo.fileName();
    }
    return QString();
}



WaitForNetwork::WaitForNetwork(enum interface iface)
{
    name = "Connect to WiFi";
    _iface = iface;
}

void WaitForNetwork::runTest()
{
    QString interface = getInterfaceName(_iface);
    if (interface == "") {
        testError("Couldn't find requested network interface");
        return;
    }

    testInfo(QString() + "Waiting for network interface " + interface + " to come up...");

    while (1) {
        foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces()) {
            // Return only the first non-loopback MAC Address
            if (!(netInterface.name() == interface))
                continue;

            if (!(netInterface.flags() & QNetworkInterface::IsRunning))
                continue;

            QList<QNetworkAddressEntry> addresses = netInterface.addressEntries();
            if (addresses.size() < 1)
                continue;

            testDebug(QString() + "Iface: " + netInterface.name());
            testDebug(QString() + "Flags: " + QString::number(netInterface.flags()));
            testDebug(QString() + "Addresses: ");
            int foundIpv4 = 0;
            for (int i = 0; i < addresses.size(); i++) {
                testDebug(addresses.at(i).ip().toString());
                if (addresses.at(i).ip().protocol() == QAbstractSocket::IPv4Protocol)
                    foundIpv4++;
            }

            if (foundIpv4 < 1)
                continue;

            testInfo("Interface " + interface + " came up");
            return;
        }
        SleeperThread::msleep(1000);
    }

    testError("Couldn't find requested interface");
    return;
}

