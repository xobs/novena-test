#include "netperftest.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QNetworkInterface>
#include <QThread>
#include <QTime>

#include <syscall.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define PKTGEN_PATH "kernel/net/core/pktgen.ko"


class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


const QString NetPerfTest::getInterfaceName(enum interface iface)
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

int NetPerfTest::initModule(const QString &modname)
{
    int fd;
    struct stat st;
    int ret;
    struct utsname ver;
    QString path;
    uname(&ver);

    testDebug(QString("Kernel release: %1  Module: %2").arg(ver.release).arg(modname));
    path = QString("/lib/modules/%1/%2").arg(ver.release).arg(modname);

    fd = open(path.toUtf8(), O_RDONLY);
    if (fd == -1) {
        testError(QString("Unable to open %1 module: %2").arg(path).arg(strerror(errno)));
        return -1;
    }

    fstat(fd, &st);

    char dat[st.st_size];
    if (read(fd, dat, sizeof(dat)) != (int)sizeof(dat)) {
        testError(QString("Couldn't read %1 module off disk: %2").arg(path).arg(strerror(errno)));
        close(fd);
        return -2;
    }
    close(fd);

    ret = init_module(dat, sizeof(dat), "");
    if (ret && errno == EEXIST)
        ret = errno = 0;
    if (ret)
        testError(QString("Couldn't insert %1 module: %2").arg(path).arg(strerror(errno)));

    return ret;
}


int NetPerfTest::setMTU(const QString &iface)
{
    QProcess ifconfig;

    testInfo("Setting MTU of interface");

    testDebug("Starting ifconfig");
    ifconfig.start("ifconfig", QStringList() << iface << "mtu" << "492");
    if (!ifconfig.waitForStarted()) {
        testError("Unable to start ifconfig");
        return 1;
    }

    ifconfig.closeWriteChannel();

    if (!ifconfig.waitForFinished()) {
        testError("ifconfig returned an error");
        testError(ifconfig.readAllStandardError());
        return 1;
    }

    if (ifconfig.exitCode()) {
        testError("ifconfig exited with a nonzero result: "
			+ QString::number(ifconfig.exitCode()));
        testError(ifconfig.readAllStandardError());
        return 1;
    }

    return 0;
}

int NetPerfTest::setIPAddress(const QString &iface, const QString &address)
{
    QProcess ifconfig;

    ifconfig.start("ifconfig", QStringList() << iface << "down");
    ifconfig.waitForFinished();

    testDebug(QString("Setting IP address of %1 to %2").arg(iface).arg(address));

    ifconfig.start("ifconfig", QStringList() << iface << address << "up");
    if (!ifconfig.waitForStarted()) {
        testError("Unable to start ifconfig");
        return 1;
    }

    ifconfig.closeWriteChannel();

    if (!ifconfig.waitForFinished()) {
        testError("ifconfig returned an error");
        testError(ifconfig.readAllStandardError());
        return 1;
    }

    if (ifconfig.exitCode()) {
        testError("ifconfig exited with a nonzero result: "
			+ QString::number(ifconfig.exitCode()));
        testError(ifconfig.readAllStandardError());
        return 1;
    }

    return 0;
}

const QString NetPerfTest::getMacAddress(const QString &iface)
{
    foreach(QNetworkInterface netInterface, QNetworkInterface::allInterfaces()) {
        // Return only the first non-loopback MAC Address
        if (netInterface.name() == iface)
            return netInterface.hardwareAddress();
    }
    return QString();
}

int NetPerfTest::pgset(const QString &device, const QString &cmd)
{

    QFile pgdev(QString("/proc/net/pktgen/") + device);
    testDebug(QString("Sending command: ") + cmd);

    if (!pgdev.open(QIODevice::WriteOnly)) {
        testError(QString() + "Unable to open pgdev " + device + ": " + pgdev.errorString());
        return 1;
    }

    QByteArray output = cmd.toUtf8();
    output += "\n";

    if (pgdev.write(QByteArray(output)) == -1) {
        testError(QString("Unable to send pgdev command: ") + pgdev.errorString());
        return 1;
    }

    pgdev.close();

    /* pgctrl file doesn't have a "Result:" string */
    if (device.contains("pgctrl"))
        return 0;

    if (!pgdev.open(QIODevice::ReadOnly)) {
        testError(QString() + "Unable to open pgdev " + device + " for verification: " + pgdev.errorString());
        return 1;
    }

    while(1) {
        QByteArray arr = pgdev.readLine();
        if (arr.length() == 0) {
            testError("Couldn't find result string");
            break;
        }
        if (arr.startsWith("Result: ")) {
            if (arr.contains("Result: OK"))
                break;
            testError(QString() + "Unable to send " + cmd + " to " + device + ": " + arr);
            break;
        }
    }

    pgdev.close();
    return 0;
}


int NetPerfTest::getNetworkBytes(const QString &iface, qulonglong &tx, qulonglong &rx)
{
    QFile procnet("/proc/net/dev");
    if (!procnet.open(QIODevice::ReadOnly)) {
        testError(QString() + "Unable to open /proc/net/dev: " + procnet.errorString());
        return 1;
    }

    while(1) {
        QString line = procnet.readLine();
        if (line.length() == 0) {
            testError(QString() + "Unable to find interface " + iface + " to get network byte count");
            procnet.close();
            return 1;
        }

        if (line.contains(iface + ":")) {
             QStringList fields = line.split(QRegExp("\\s+"));
             rx = fields.at(2).toULongLong();
             tx = fields.at(10).toULongLong();
             procnet.close();
             return 0;
        }
    }
    procnet.close();

    testError("Exited getNetworkBytes loop somehow");
    return 0;
}



NetPerfTest::NetPerfTest()
{
    name = "Network Performance Test";
}

void NetPerfTest::runTest()
{
    QString gbit = getInterfaceName(GBit);
    QString usb = getInterfaceName(USB);
    QString wifi = getInterfaceName(WiFi);

    QString gbitMac;
    QString wifiMac;
    QString usbMac;

    QString gbitIp = "172.16.5.4";
    QString usbIp = "172.16.5.6";

    QString pgdev;
    int pktlen;
    int pktcount = 100000;
    QString rate = "70M";

    qulonglong usbTxStart, usbTxEnd, usbRxStart, usbRxEnd;
    qulonglong gbitTxStart, gbitTxEnd, gbitRxStart, gbitRxEnd;

    QTime timer;


    testInfo(QString("Gigabit is: %1").arg(gbit));
    testInfo(QString("USB is: %1").arg(usb));
    testInfo(QString("Wifi is: %1").arg(wifi));

    if (usb == "") {
        testError("Unable to find USB Ethernet port");
        return;
    }

    if (gbit == "") {
        testError("Unable to find gigabit Ethernet port");
        return;
    }

    if (wifi == "") {
        testError("Unable to find Wifi card");
        return;
    }


    gbitMac = getMacAddress(gbit);
    testDebug(QString("Gigabit MAC address: ") + gbitMac);

    wifiMac = getMacAddress(wifi);
    testDebug(QString("WiFi MAC address: ") + wifiMac);

    usbMac = getMacAddress(usb);
    testDebug(QString("USB MAC address: ") + usbMac);

    if (initModule(PKTGEN_PATH))
        return;


    if (setIPAddress(gbit, gbitIp)) {
        testError("Unable to assign an address to gigabit Ethernet port");
        return;
    }

    if (setIPAddress(usb, usbIp)) {
        testError("Unable to assign an address to USB Ethernet port");
        return;
    }

    /*
     * The interface takes a while to come up, and there doesn't
     * seem to be any way to determine if it's active or not.
     */
    SleeperThread::msleep(5000);


    /* Remove all devices from both threads, so they're not in use */
    pgdev = "kpktgend_0";
    pgset(pgdev, "rem_device_all");
    pgdev = "kpktgend_1";
    pgset(pgdev, "rem_device_all");
    pgdev = "kpktgend_2";
    pgset(pgdev, "rem_device_all");
    pgdev = "kpktgend_3";
    pgset(pgdev, "rem_device_all");


    testDebug("Adding devices to run");

    pgdev = "kpktgend_0";
    pgset(pgdev, QString("add_device ") + usb);

    pgdev = "kpktgend_2";
    pgset(pgdev, QString("add_device ") + gbit);



    testInfo("Configuring individual devices");

    testDebug("Configuring generator for USB");
    pgdev = usb;
    pktlen = 1300;
    pgset(pgdev, "clone_skb 0");
    pgset(pgdev, QString("min_pkt_size ") + QString::number(pktlen));
    pgset(pgdev, QString("max_pkt_size ") + QString::number(pktlen));
    pgset(pgdev, QString("dst ") + gbitIp);
    pgset(pgdev, QString("dst_mac ") + gbitMac);
    pgset(pgdev, QString("count ") + QString::number(pktcount));
    pgset(pgdev, QString("rate ") + rate);
    pgset(pgdev, "delay 100000");

    testDebug("Configuring generator for Gigabit");
    pgdev = gbit;
    pktlen = 1300;
    pgset(pgdev, "clone_skb 0");
    pgset(pgdev, QString("min_pkt_size ") + QString::number(pktlen));
    pgset(pgdev, QString("max_pkt_size ") + QString::number(pktlen));
    pgset(pgdev, QString("dst ") + usbIp);
    pgset(pgdev, QString("dst_mac ") + usbMac);
    pgset(pgdev, QString("count ") + QString::number(pktcount));
    pgset(pgdev, QString("rate ") + rate);
    pgset(pgdev, "delay 100000");


    getNetworkBytes(usb, usbTxStart, usbRxStart);
    getNetworkBytes(gbit, gbitTxStart, gbitRxStart);
    testInfo("Starting packet generator");
    pgdev = "pgctrl";

    timer.start();
    pgset(pgdev, "start");
    int elapsed = timer.elapsed();

    getNetworkBytes(usb, usbTxEnd, usbRxEnd);
    getNetworkBytes(gbit, gbitTxEnd, gbitRxEnd);

    testInfo(QString("Test completed in ") + QString::number(elapsed) + " ms");
    qulonglong usbTxRate = ((((usbTxEnd - usbTxStart) * 8 * 1000) / elapsed) / 1024) / 1024;
    qulonglong usbRxRate = ((((usbRxEnd - usbRxStart) * 8 * 1000) / elapsed) / 1024) / 1024;
    qulonglong gbitTxRate = ((((gbitTxEnd - gbitTxStart) * 8 * 1000) / elapsed) / 1024) / 1024;
    qulonglong gbitRxRate = ((((gbitRxEnd - gbitRxStart) * 8 * 1000) / elapsed) / 1024) / 1024;
    testInfo(QString() + "USB Tx/Rx: " + QString::number(usbTxRate) + "/" + QString::number(usbRxRate) + " Mbit/sec");
    testInfo(QString() + "Gbit Tx/Rx: " + QString::number(gbitTxRate) + "/" + QString::number(gbitRxRate) + " Mbit/sec");
    

    if (usbTxRate < 50)
        testError("USB Tx rate too low");

    if (usbRxRate < 50)
        testError("USB Rx rate too low");

    if (gbitTxRate < 50)
        testError("Gigabit Tx rate too low");

    if (gbitRxRate < 50)
        testError("Gigabit Rx rate too low");

}
