#include "packageinstaller.h"

#include <QProcess>
#include <sys/mount.h>
#include <errno.h>
#define MOUNT_POINT "/tmp/factory-test-chroot"

PackageInstaller::PackageInstaller(const QString _disk, const QString dir, const QStringList package_list)
{
    name = "Install Packages";

    path = MOUNT_POINT;
    disk = _disk;

    /* Construct a string: dpkg --root=[root] -i [package-list] */
    packages.append(QString("--root=").append(MOUNT_POINT));
    packages.append("-i");
    foreach (QString pkg, package_list) {
        packages.append(QString(dir).append(pkg));
    }
}

int PackageInstaller::mountDisk()
 {
    QString ext4fs = QString("%1-part3").arg(disk);
    QString vfatfs = QString("%1-part1").arg(disk);
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    testInfo("Mounting devices");
    system("mkdir -p " MOUNT_POINT);

    testDebug("Mounting root filesystem");
    if (mount(ext4fs.toUtf8(), MOUNT_POINT, "ext4", MS_NOATIME | MS_NODIRATIME, "data=writeback,barrier=0,errors=remount-ro") == -1) {
        testError("Unable to mount image");
        return -1;
    }

    testDebug("Mounting boot filesystem");
    if (mount(vfatfs.toUtf8(), bootPath.toUtf8(), "vfat", MS_NOATIME | MS_NODIRATIME, "") == -1) {
        testError("Unable to mount image boot directory");
        umount(MOUNT_POINT);
        return -1;
    }

    return 0;
}

int PackageInstaller::unmountDisk()
{
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    testInfo("Unmounting disk");

    testDebug("Unmounting boot partition");
    if (umount(bootPath.toUtf8()) == -1) {
        testError(QString("Unable to umount /boot: %1").arg(strerror(errno)));
        return -1;
    }

    testDebug("Unmounting root partition");
    if (umount(MOUNT_POINT) == -1) {
        testError(QString("Unable to umount /: %1").arg(strerror(errno)));
        return -1;
    }

    return 0;
}

void PackageInstaller::runTest()
{
    QProcess dpkg;

    mountDisk();

    testInfo(QString("Beginning dpkg ").append(packages.join(" ")));

    dpkg.setProcessChannelMode(QProcess::MergedChannels);
    dpkg.start("/usr/bin/dpkg", packages);

    dpkg.waitForStarted();
    while(dpkg.state() == QProcess::Running) {
        QByteArray data = dpkg.readLine();
        if (data.size())
            testDebug(data);
        dpkg.waitForReadyRead();
    }

    if (dpkg.exitCode())
        testError(QString("dpkg returned an error: %1").arg(QString::number(dpkg.exitCode())));

    system("cp " MOUNT_POINT "/boot/zimage " MOUNT_POINT "/boot/zimage.recovery");
    system("cp " MOUNT_POINT "/boot/novena.dtb " MOUNT_POINT "/boot/novena.recovery.dtb");

    unmountDisk();
}
