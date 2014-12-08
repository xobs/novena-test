#include "destructivedisktest.h"

DestructiveDiskTest::DestructiveDiskTest(const QString _path, const QString _type) : path(_path)
{
    name = QString("Destructive Disk Test: %1").arg(_type);
}

void DestructiveDiskTest::runTest()
{
}
