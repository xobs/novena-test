#include <QThread>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QTime>
#include <stdint.h>
#ifdef linux
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif
#include "mmctest.h"

static MMCCopyThread *copyThread;
static QTime *timer;

#define MOUNT_POINT "/mnt/"

#define errorOut(x) \
    do { \
        emit copyError(x); \
        return 1; \
    } while(0)


MMCCopyThread::MMCCopyThread(QString src,  QString dst)
    : outputImage(dst), inputImage(src)
{
    source = src;
    destination = dst;
}

void MMCCopyThread::infoMessage(QString msg)
{
    emit copyInfo(msg);
}

void MMCCopyThread::debugMessage(QString msg)
{
    emit copyDebug(msg);
}

QString MMCCopyThread::getInternalBlockName()
{
    return "/dev/disk/by-path/platform-2198000.usdhc";
}

QString MMCCopyThread::getExternalBlockName()
{
    return "/dev/disk/by-path/platform-2194000.usdhc";
}


int MMCCopyThread::findDevices()
{
    infoMessage(QString("Opening input image %1").arg(inputImage.fileName()));
    if (!inputImage.open(QIODevice::ReadOnly))
        errorOut("Couldn't open input image file");

    infoMessage(QString("Opening output image %1").arg(outputImage.fileName()));
    if (!outputImage.open(QIODevice::WriteOnly| QIODevice::Truncate))
        errorOut("Couldn't open output image file");

    return 0;
}

int MMCCopyThread::extractImage()
{
    qint64 ret;
    char data[1024 * 1024];

    while ((ret = inputImage.read(data, sizeof(data))) > 0) {
        ret = outputImage.write(data, ret);

        if (ret <= 0)
            errorOut("Unable to write data");
    }

    if (ret == -1)
        errorOut("Unable to read data");

    return 0;
}

int MMCCopyThread::resizeMBR()
{
    QProcess fdisk;

    infoMessage("Resizing disk");

    debugMessage("Starting fdisk");
    fdisk.start("fdisk", QStringList() << outputImage.fileName());
    if (!fdisk.waitForStarted())
        errorOut("Unable to start fdisk");

    fdisk.write("d\n"); /* Delete existing partition */
    fdisk.write("3\n"); /* Partition number 3 */
    fdisk.write("n\n"); /* New partition */
    fdisk.write("p\n"); /* Primary type */
    fdisk.write("3\n"); /* Partition number 3 */
    fdisk.write(" \n"); /* Default starting offset */
    fdisk.write(" \n"); /* Default ending offset */
    fdisk.write("w\n"); /* Write changes to disk */

    if (!fdisk.waitForFinished())
        errorOut("fdisk returned an error");

    return 0;
}

int MMCCopyThread::resizeRoot()
{


    infoMessage("Updating partition table");
    if (!outputImage.flush())
        errorOut("Unable to sync disk");
    if (ioctl(outputImage.handle(), BLKRRPART, NULL) == -1)
        errorOut(QString("Unable to re-read MBR: %1").arg(strerror(errno)));
    sleep(10);


    QProcess fsck;
    infoMessage("Running fsck.ext4");

    fsck.start("fsck.ext4", QStringList() << "-y" << QString("%1-part3").arg(outputImage.fileName()));

    if (!fsck.waitForStarted())
        errorOut("Unable to start fsck");

    fsck.closeWriteChannel();

    if (!fsck.waitForFinished(INT_MAX)) {
        copyError(fsck.readAllStandardError());
        errorOut("fsck returned an error");
    }

    if (fsck.exitCode()) {
        copyError(fsck.readAllStandardError());
        errorOut(QString("fsck returned an error: " + QString::number(fsck.exitCode())));
    }


    sleep(10);


    QProcess resize2fs;
    infoMessage("Checking disk");

    resize2fs.start("resize2fs", QStringList() << "-f" << QString("%1-part3").arg(outputImage.fileName()));

    if (!resize2fs.waitForStarted())
        errorOut("Unable to start resize2fs");

    resize2fs.closeWriteChannel();

    if (!resize2fs.waitForFinished(INT_MAX)) {
        copyError(resize2fs.readAllStandardError());
        errorOut("resize2fs returned an error");
    }

    if (resize2fs.exitCode()) {
        copyError(resize2fs.readAllStandardError());
        errorOut(QString("resize2fs returned an error: " + QString::number(resize2fs.exitCode())));
    }

    return 0;
}

int MMCCopyThread::mountDisk()
 {
    QString ext4fs = QString("%1-part3").arg(outputImage.fileName());
    QString vfatfs = QString("%1-part1").arg(outputImage.fileName());
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    infoMessage("Mounting devices");

    debugMessage("Mounting root filesystem");
    if (mount(ext4fs.toLocal8Bit(), MOUNT_POINT, "ext4", MS_NOATIME | MS_NODIRATIME, "data=writeback,barrier=0,errors=remount-ro") == -1)
        errorOut("Unable to mount image");

    debugMessage("Mounting boot filesystem");
    if (mount(vfatfs.toLocal8Bit(), bootPath.toLocal8Bit(), "vfat", MS_NOATIME | MS_NODIRATIME, "") == -1) {
        errorOut("Unable to mount image boot directory");
        umount(MOUNT_POINT);
    }

    return 0;
}

int MMCCopyThread::unmountDisk()
{
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    infoMessage("Unmounting disk");

    debugMessage("Unmounting boot partition");
    if (umount(bootPath.toLocal8Bit()) == -1)
        errorOut(QString("Unable to umount /boot: %1").arg(strerror(errno)));

    debugMessage("Unmounting root partition");
    if (umount(MOUNT_POINT) == -1)
        errorOut(QString("Unable to umount /: %1").arg(strerror(errno)));

    outputImage.close();
    inputImage.close();

    return 0;
}

void MMCCopyThread::run()
{
    if (findDevices())
        return;
    if (extractImage())
        return;
    if (resizeMBR())
        return;
    if (resizeRoot())
        return;
    if (mountDisk())
        return;
    if (unmountDisk())
        return;
}


MMCTestStart::MMCTestStart(QString src, QString dst)
{
    name = "MMC imaging";
    copyThread = new MMCCopyThread(src, dst);

    connect(copyThread, SIGNAL(copyError(QString)),
            this, SLOT(testError(QString)));
    connect(copyThread, SIGNAL(copyInfo(QString)),
            this, SLOT(testInfo(QString)));
    connect(copyThread, SIGNAL(copyDebug(QString)),
            this, SLOT(testDebug(QString)));

    timer = new QTime();
}

void MMCTestStart::runTest()
{
    testInfo(QString("Checking MMC status..."));
    if (copyThread->isRunning()) {
        testError("Critical error: MMC test is already running");
        return;
    }

    testInfo("Starting MMC copy");
    timer->start();
    copyThread->start();
    testInfo("MMC copy continuing in background");
}



MMCTestFinish::MMCTestFinish()
{
    name = "Finalizing MMC";
}

void MMCTestFinish::runTest()
{
    testInfo("Waiting for MMC to finish...");
    copyThread->wait();
    testDebug(QString("Image writing took %1 ms").arg(timer->elapsed()));
    testInfo("Done");
}
