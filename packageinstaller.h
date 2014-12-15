#ifndef PACKAGEINSTALLER_H
#define PACKAGEINSTALLER_H
#include "novenatest.h"

#include <QStringList>

class PackageInstaller : public NovenaTest
{
public:
    PackageInstaller(const QString dir, const QStringList package_list);
    void runTest();

private:
    QStringList packages;
};

#endif // PACKAGEINSTALLER_H
