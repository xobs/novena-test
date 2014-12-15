#include "packageinstaller.h"

#include <QProcess>

PackageInstaller::PackageInstaller(const QString dir, const QStringList package_list)
{
    name = "Install Packages";

    packages.append("-i");
    foreach (QString pkg, package_list) {
        packages.append(QString(dir).append(pkg));
    }
}

void PackageInstaller::runTest()
{
    QProcess dpkg;

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
}
