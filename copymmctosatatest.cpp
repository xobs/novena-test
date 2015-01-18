#include <QFile>
#include <QProcess>

#include "copymmctosatatest.h"

#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MOUNT_POINT "/mnt/"

int CopyMMCToSataTest::resizeMBR(void)
{
    QProcess fdisk;

    testInfo("Resizing disk");

    testDebug("Starting fdisk");
    fdisk.start("fdisk", QStringList() << dst.fileName());
    if (!fdisk.waitForStarted()) {
        testError("Unable to start fdisk");
        return 1;
    }

    fdisk.write("d\n"); /* Delete existing partition */
    fdisk.write("3\n"); /* Partition number 3 */
    fdisk.write("n\n"); /* New partition */
    fdisk.write("p\n"); /* Primary type */
    fdisk.write("3\n"); /* Partition number 3 */
    fdisk.write(" \n"); /* Default starting offset */
    fdisk.write(" \n"); /* Default ending offset */
    fdisk.write("x\n"); /* Expert mode */
    fdisk.write("i\n"); /* Change disk ID */
    fdisk.write("0x4e6f7653\n"); /* NovS */
    fdisk.write("r\n"); /* Return to main menu */
    fdisk.write("w\n"); /* Write changes to disk */

    if (!fdisk.waitForFinished()) {
        testError("fdisk returned an error");
        return 1;
    }

    testInfo("Updating partition table");

    if (!dst.flush()) {
        testError("Unable to sync disk");
        return 1;
    }

    if (ioctl(dst.handle(), BLKRRPART, NULL) == -1) {
        testError(QString("Unable to re-read MBR: %1").arg(strerror(errno)));
        return 1;
    }

    return 0;
}

int CopyMMCToSataTest::resizeRoot(void)
{

    sleep(10);


    QProcess fsck;
    testInfo("Running fsck.ext4");

    fsck.start("fsck.ext4", QStringList() << "-y" << QString("%1%2").arg(dst.fileName()).arg("3"));

    if (!fsck.waitForStarted()) {
        testError("Unable to start fsck");
        return 1;
    }

    fsck.closeWriteChannel();

    if (!fsck.waitForFinished(INT_MAX)) {
        testError("fsck returned an error");
        testError(fsck.readAllStandardError());
        return 1;
    }

    if (fsck.exitCode()) {
        testError(QString("fsck returned an error: " + QString::number(fsck.exitCode())));
        return 1;
    }

    sync();
    sleep(10);


    QProcess resize2fs;
    testInfo("Checking disk");

    resize2fs.start("resize2fs", QStringList() << "-f" << QString("%1%2").arg(dst.fileName()).arg("3"));

    if (!resize2fs.waitForStarted()) {
        testError("Unable to start resize2fs");
        return 1;
    }

    resize2fs.closeWriteChannel();

    if (!resize2fs.waitForFinished(INT_MAX)) {
        testError("resize2fs returned an error");
        testError(resize2fs.readAllStandardError());
        return 1;
    }

    if (resize2fs.exitCode()) {
        testError(QString("resize2fs returned an error: " + QString::number(resize2fs.exitCode())));
        testError(resize2fs.readAllStandardError());
        return 1;
    }

    sync();

    return 0;
}

int CopyMMCToSataTest::mountDisk(void)
 {
    QString ext4fs = QString("%1%2").arg(dst.fileName()).arg("3");
    QString vfatfs = QString("%1%2").arg(dst.fileName()).arg("1");
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    testInfo("Mounting devices");

    testDebug("Mounting root filesystem");
    if (mount(ext4fs.toLocal8Bit(), MOUNT_POINT, "ext4", MS_NOATIME | MS_NODIRATIME, "data=writeback,barrier=0,errors=remount-ro") == -1) {
        testError("Unable to mount image");
        return 1;
    }

    testDebug("Mounting boot filesystem");
    if (mount(vfatfs.toLocal8Bit(), bootPath.toLocal8Bit(), "vfat", MS_NOATIME | MS_NODIRATIME, "") == -1) {
        testError("Unable to mount image boot directory");
        umount(MOUNT_POINT);
        return 1;
    }

    return 0;
}

int CopyMMCToSataTest::unmountDisk(void)
{
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    testInfo("Unmounting disk");

    testDebug("Unmounting boot partition");
    if (umount(bootPath.toLocal8Bit()) == -1) {
        testError(QString("Unable to umount /boot: %1").arg(strerror(errno)));
        return 1;
    }

    testDebug("Unmounting root partition");
    if (umount(MOUNT_POINT) == -1) {
        testError(QString("Unable to umount /: %1").arg(strerror(errno)));
        return 1;
    }

    return 0;
}

CopyMMCToSataTest::CopyMMCToSataTest(QString _src, QString _dst)
    : src(_src), dst(_dst)
{
    name = "Copy MMC to Sata";
}

void CopyMMCToSataTest::runTest()
{
    src.open(QIODevice::ReadOnly);
    if (!src.isOpen()) {
        testError(QString("Unable to open MMC for reading: %1").arg(src.errorString()));
        goto done;
    }

    dst.open(QIODevice::WriteOnly);
    if (!dst.isOpen()) {
        testError(QString("Unable to open Sata for writing: %1").arg(dst.errorString()));
        goto done;
    }

    testInfo("Beginning copy...");

    while (1) {
        QByteArray data = src.read(1024 * 1024);
        if (src.error() != QFileDevice::NoError) {
            testError(QString("Unable to read data from MMC: %1").arg(src.errorString()));
            goto done;
        }

        if (data.size() == 0)
            break;

        if (dst.write(data) != data.size()) {
            testError(QString("Unable to write data to Sata: %1").arg(dst.errorString()));
            goto done;
        }
    }

    if (resizeMBR())
        goto done;
    if (resizeRoot())
        goto done;
    if (mountDisk())
        goto done;
    if (unmountDisk())
        goto done;

done:
    dst.close();
    src.close();
}
