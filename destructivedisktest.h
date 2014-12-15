#ifndef DESETRUCTIVEDISKTEST_H
#define DESETRUCTIVEDISKTEST_H
#include "novenatest.h"

class DestructiveDiskTest : public NovenaTest
{
public:
    DestructiveDiskTest(quint32 _bytes, const QString _path, const QString _type);
    void runTest();

private:
    QString path;
    quint32 bytes;
};

#endif // DESETRUCTIVEDISKTEST_H
