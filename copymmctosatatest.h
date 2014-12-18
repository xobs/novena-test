#ifndef COPYMMCTOSATATEST_H
#define COPYMMCTOSATATEST_H
#include "novenatest.h"

#include <QFile>

class CopyMMCToSataTest : public NovenaTest
{
public:
    CopyMMCToSataTest(QString src, QString dst);
    void runTest();

private:
    QFile src;
    QFile dst;

    int resizeMBR(void);
    int resizeRoot(void);
    int mountDisk(void);
    int unmountDisk(void);
};

#endif // COPYMMCTOSATATEST_H
