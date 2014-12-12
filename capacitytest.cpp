#include <QFile>
#include <QFileInfo>
#include <QString>
#include "capacitytest.h"
#include <QDebug>

CapacityTest::CapacityTest(enum device _device, int _min_size_kb, int _max_size_kb)
{
    name = QString("Disk Capacity Test");
    max_size = _max_size_kb * 1024LL;
    min_size = _min_size_kb * 1024LL;
    device = _device;
}

void CapacityTest::runTest()
{
    const char *devname;

    if (device == InternalDevice)
        devname = "/dev/disk/by-path/platform-2198000.usdhc";
    else
        devname = "/dev/disk/by-path/platform-2194000.usdhc";

    QFileInfo pathFile(devname);
    QFileInfo realFile(pathFile.symLinkTarget());
    testDebug(QString("MMC device is ") + realFile.fileName());

    QFile sizeFile(QString("/sys/class/block/%1/size").arg(realFile.fileName()));
    sizeFile.open(QIODevice::ReadOnly);

    QString sizeStr = sizeFile.readAll().trimmed();
    qint64 size = sizeStr.toLongLong() * 512;

    testInfo(QString("Capacity is %1 blocks (%2 bytes, %3 KB, %4 MB, %5 GB)")
             .arg(sizeStr)
             .arg(QString::number(size))
             .arg(QString::number(size / 1024))
             .arg(QString::number(size / 1024 / 1024))
             .arg(QString::number(size / 1024 / 1024 / 1024)));

    if ((min_size >= 0) && (size < min_size))
        testError("Disk is too small");
    else
        testDebug(QString("Minimum size is %1, which is bigger than %2").arg(QString::number(min_size)).arg(QString::number(size)));

    if ((max_size >= 0) && (size > max_size))
        testError("Disk is too large");
    else
        testDebug(QString("Maximum size is %1, which is smaller than %2").arg(QString::number(max_size)).arg(QString::number(size)));
}
