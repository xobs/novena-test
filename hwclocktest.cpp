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
#include "hwclocktest.h"

#define errorOut(x) \
do { \
	testError(x); \
	return 1; \
} while(0)



HWClockTestStart::HWClockTestStart(void)
{
    name = "Capture HW Clock";
}

int HWClockTestStart::getNtpDate(void)
{
    QProcess ntpdate;
    testInfo("Obtaining time from network");

    ntpdate.start("ntpdate-debian");

    if (!ntpdate.waitForStarted())
        errorOut("Unable to start ntpdate");

    ntpdate.closeWriteChannel();

    if (!ntpdate.waitForFinished(INT_MAX)) {
        testError(ntpdate.readAllStandardError());
        errorOut("ntpdate returned an error");
    }

    if (ntpdate.exitCode()) {
        testError(ntpdate.readAllStandardError());
        errorOut(QString("ntpdate returned an error: " + QString::number(ntpdate.exitCode())));
    }

    return 0;
}

int HWClockTestStart::sysToRTC(void)
{
    QProcess hwclock;
    testInfo("Obtaining time from network");

    hwclock.start("hwclock", QStringList() << "-w");

    if (!hwclock.waitForStarted())
        errorOut("Unable to start hwclock");

    hwclock.closeWriteChannel();

    if (!hwclock.waitForFinished(INT_MAX)) {
        testError(hwclock.readAllStandardError());
        errorOut("hwclock returned an error");
    }

    if (hwclock.exitCode()) {
        testError(hwclock.readAllStandardError());
        errorOut(QString("hwclock returned an error: " + QString::number(hwclock.exitCode())));
    }

    return 0;
}

void HWClockTestStart::runTest()
{
    testInfo("Capturing HW clock value...");
    QFile rtcNameFile("/sys/class/rtc/rtc0/name");
    if (!rtcNameFile.open(QIODevice::ReadOnly)) {
        testError("Unable to open RTC file");
        return;
    }

    QString rtcName = rtcNameFile.readAll();
    rtcNameFile.close();

    if (rtcName != "rtc-pcf8523\n") {
        testError(QString("RTC is named '%1', not 'rtc-pcf8523\\n'").arg(rtcName));
        return;
    }

    if (getNtpDate())
        return;

    testInfo("MMC copy continuing in background");
}



HWClockTestFinish::HWClockTestFinish()
{
    name = "Validating HW Clock";
}

void HWClockTestFinish::runTest()
{
    QFile rtcEpochFile("/sys/class/rtc/rtc0/since_epoch");
    if (!rtcEpochFile.open(QIODevice::ReadOnly)) {
        testError("Unable to open RTC epoch");
        return;
    }

    QString rtcEpochStr = rtcEpochFile.readAll();
    rtcEpochFile.close();

    int rtcEpoch = rtcEpochStr.toInt();
    int sysEpoch = time(NULL);

    testInfo(QString("HW RTC epoch is %1").arg(rtcEpoch));
    testInfo(QString("System epoch is %1").arg(sysEpoch));

    if (abs(rtcEpoch - sysEpoch) > 5)
        testError("Drift between RTC and system clock is too great");
    else
        testInfo("RTC drift is acceptable");
}
