#ifndef PACKAGEINSTALLER_H
#define PACKAGEINSTALLER_H
#include "novenatest.h"

#include <QStringList>

class PackageInstaller : public NovenaTest
{
public:
    PackageInstaller(const QString disk, const QString dir, const QStringList package_list);
    void runTest();

private:
    QStringList packages;
    QString disk;
    QString path;

    int mountDisk();
    int unmountDisk();
};

#endif // PACKAGEINSTALLER_H
