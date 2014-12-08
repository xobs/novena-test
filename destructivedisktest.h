#ifndef DESETRUCTIVEDISKTEST_H
#define DESETRUCTIVEDISKTEST_H
#include "novenatest.h"

class DestructiveDiskTest : public NovenaTest
{
public:
    DestructiveDiskTest(const QString _path, const QString _type);
    void runTest();

private:
    QString path;
};

#endif // DESETRUCTIVEDISKTEST_H
