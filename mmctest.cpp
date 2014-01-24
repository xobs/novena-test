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

#define BOOT_PART_SIZE 32   /* Megabytes */
#define ROOT_PART_SIZE 3500 /* Megabytes */
#define MOUNT_POINT "/mnt/"

struct part_entry {
    uint8_t status;
    uint8_t chs1;
    uint8_t chs2;
    uint8_t chs3;
    uint8_t type;
    uint8_t chs4;
    uint8_t chs5;
    uint8_t chs6;
    uint32_t lba_address;
    uint32_t lba_size;
} __attribute__((__packed__));

struct mbr {
    uint8_t code[440]; /* 0 - 440 */
    uint8_t disk_signature[4]; /* 440 - 444 */
    uint8_t reserved[2]; /* 444 - 446 */
    struct part_entry partitions[4]; /* 446 - 510 */
    uint8_t signature[2]; /* 510 - 512 */
} __attribute__((__packed__));

static struct part_args {
    uint32_t start;
    uint32_t size;
    uint32_t type;
    uint32_t padding_pre; /* How much space to leave before the partition */
    char *file;
} parts[4] = {
    {
        .size = BOOT_PART_SIZE * 1024 * (1024 / 512),
        .type = 0x06,
        .padding_pre = 256 * 1024 / 512,
    },
    {
        .size = ROOT_PART_SIZE * 1024 * (1024 / 512),
        .type = 0x82,
    },
};

#define errorOut(x) \
    do { \
        emit copyError(x); \
        return 1; \
    } while(0)


MMCCopyThread::MMCCopyThread(QString src, QString bl, QString dst)
    : outputImage(dst), inputImage(src), bootloaderImage(bl)
{
    source = src;
    destination = dst;
    bootloader = bl;
}

void MMCCopyThread::infoMessage(QString msg)
{
    emit copyInfo(msg);
}

void MMCCopyThread::debugMessage(QString msg)
{
    emit copyDebug(msg);
}

QString MMCCopyThread::getBlockName(QString mmcNumber)
{
    QDir mmc = QString("/sys/class/mmc_host/%1/").arg(mmcNumber);
    QString mmcColon = QString("%1:").arg(mmcNumber);
    QFileInfoList list = mmc.entryInfoList();

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().startsWith(mmcColon)) {
            QString block = QString("%1/block/").arg(fileInfo.absoluteFilePath());
            QDir blockDir(block);

            QFileInfoList blockList = blockDir.entryInfoList();
            for (int j = 0; j < blockList.size(); ++j) {
                QFileInfo blockFileInfo = blockList.at(j);
                if (blockFileInfo.fileName().startsWith("mmcblk"))
                    return QString("/dev/%1").arg(blockFileInfo.fileName());
            }
        }
    }
    return QString();
}

QString MMCCopyThread::getInternalBlockName()
{
    return getBlockName("mmc1");
}

QString MMCCopyThread::getExternalBlockName()
{
    return getBlockName("mmc0");
}


int MMCCopyThread::findDevices()
{
    infoMessage(QString("Opening bootloader image %1").arg(bootloaderImage.fileName()));
    if (!bootloaderImage.open(QIODevice::ReadOnly))
        errorOut("Couldn't open bootloader image");

    infoMessage(QString("Opening input image %1").arg(inputImage.fileName()));
    if (!inputImage.open(QIODevice::ReadOnly))
        errorOut("Couldn't open input image file");

    infoMessage(QString("Opening output image %1").arg(outputImage.fileName()));
    if (!outputImage.open(QIODevice::WriteOnly| QIODevice::Truncate))
        errorOut("Couldn't open output image file");

    return 0;
}

int MMCCopyThread::partitionDevice()
{
    struct mbr mbr;
    uint64_t running_address = 4;
    int i;

    infoMessage("Partitioning device");

    memset(&mbr, 0, sizeof(mbr));
    mbr.signature[0] = 0x55;
    mbr.signature[1] = 0xAA;

    /* Make first partition bootable */
    mbr.partitions[0].status = 0x80;


    for (i = 0; i < 4; i++) {
        if (parts[i].size || (parts[i].type == 0x05)) {
            running_address += parts[i].padding_pre;
            parts[i].start = running_address;

            mbr.partitions[i].lba_address = parts[i].start;
            mbr.partitions[i].lba_size = parts[i].size;
            mbr.partitions[i].type = parts[i].type;

            running_address += parts[i].size;
        }
    }

    QByteArray mbrBytes((char *)&mbr, sizeof(mbr));
    if (outputImage.write(mbrBytes) == -1)
        errorOut("Unable to write MBR");

    /*
     * Write an additional 512 bytes, moving the cursor to byte 1024.
     * This is so that when we write the bootloader later on, it ends
     * up at the right offset.
     */
    QByteArray zeroes(512, 0);
    if (outputImage.write(zeroes) == -1)
        errorOut("Unable to pad image");

#ifdef linux
    if (ioctl(outputImage.handle(), BLKRRPART, NULL) == -1)
        errorOut(QString("Unable to re-read MBR: %1").arg(strerror(errno)));
#endif

    return 0;
}

int MMCCopyThread::formatDevice()
{
    QProcess mkfs;

    infoMessage("Formatting filesystems");

    debugMessage("Starting mkfs.ext4");
    mkfs.start("mkfs.ext4", QStringList() << QString("%1p2").arg(outputImage.fileName()));
    if (!mkfs.waitForStarted())
        errorOut("Unable to start mkfs.ext4");

    mkfs.closeWriteChannel();

    if (!mkfs.waitForFinished())
        errorOut("mkfs.ext4 returned an error");

    debugMessage("Starting mkfs.vfat");
    mkfs.start("mkfs.vfat", QStringList() << QString("%1p1").arg(outputImage.fileName()));
    if (!mkfs.waitForStarted())
        errorOut("Unable to start mkfs.vfat");

    mkfs.closeWriteChannel();

    if (!mkfs.waitForFinished())
        errorOut("mkfs.vfat returned an error");

    return 0;
}

int MMCCopyThread::loadBootloader()
{
    QByteArray bl = bootloaderImage.readAll();

    infoMessage("Copying bootloader");

    if (bl.size() < 16384)
        errorOut(QString("Bootloader data is suspiciously small, at %1 bytes").arg(bl.size()));

    /*
     * For i.MX6, the bootloader belongs at offset 1024 from the start of the disk.
     * The write cursor should be left there from the disk partitioning process.
     */

    if (outputImage.write(bl) <= 0)
        errorOut("Unable to write bootloader image");

    return 0;
}

int MMCCopyThread::mountDevice()
{
#ifdef linux
    QString ext4fs = QString("%1p2").arg(outputImage.fileName());
    QString vfatfs = QString("%1p1").arg(outputImage.fileName());
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    infoMessage("Mounting devices");

    debugMessage("Mounting root filesystem");
    if (mount(ext4fs.toLocal8Bit(), MOUNT_POINT, "ext4", MS_NOATIME | MS_NODIRATIME, "data=writeback,barrier=0,errors=remount-ro") == -1)
        errorOut("Unable to mount image");

    debugMessage("Making /boot directory");
    if (mkdir(bootPath.toLocal8Bit(), 0660) == -1) {
        umount(MOUNT_POINT);
        errorOut(QString("Unable to make boot directory \"%1\": %2").arg(bootPath).arg(strerror(errno)));
    }

    debugMessage("Mounting boot filesystem");
    if (mount(vfatfs.toLocal8Bit(), bootPath.toLocal8Bit(), "vfat", MS_NOATIME | MS_NODIRATIME, "") == -1) {
        errorOut("Unable to mount image boot directory");
        umount(MOUNT_POINT);
    }
#endif
    return 0;
}

int MMCCopyThread::extractImage()
{
    QProcess tar;
    infoMessage("Extracting filesystem");

    tar.setWorkingDirectory(MOUNT_POINT);
    tar.start("tar", QStringList() << "xzf" << inputImage.fileName());
    if (!tar.waitForStarted())
        errorOut("Unable to start tar");

    tar.closeWriteChannel();

    if (!tar.waitForFinished(INT_MAX))
        errorOut("tar returned an error");

    return 0;
}

int MMCCopyThread::unmountDevice()
{
    QString bootPath = QString("%1/boot").arg(MOUNT_POINT);

    infoMessage("Unmounting devices");

    debugMessage("Unmounting boot partition");
    if (umount(bootPath.toLocal8Bit()) == -1)
        errorOut(QString("Unable to umount /boot: %1").arg(strerror(errno)));

    debugMessage("Unmounting root partition");
    if (umount(MOUNT_POINT) == -1)
        errorOut(QString("Unable to umount /: %1").arg(strerror(errno)));

    bootloaderImage.close();
    outputImage.close();
    inputImage.close();

    return 0;
}

void MMCCopyThread::run()
{
    if (findDevices())
        return;
    if (partitionDevice())
        return;
    if (formatDevice())
        return;
    if (loadBootloader())
        return;
    if (mountDevice())
        return;
    if (extractImage())
        return;
    if (unmountDevice())
        return;
}





MMCTestStart::MMCTestStart(QString src, QString bl, QString dst)
{
    name = "MMC imaging";
    copyThread = new MMCCopyThread(src, bl, dst);

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
