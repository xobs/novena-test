#ifndef MMCTEST_H
#define MMCTEST_H
#include <QThread>
#include <QFile>
#include "novenatest.h"

class MMCCopyThread;

class MMCCopyThread : public QThread {
    Q_OBJECT

private:
    QString source;
    QString destination;
    QFile outputImage;
    QFile inputImage;

    int findDevices();
    int extractImage();
    int updatePartitions();
    int resizeMBR();
    int resizeRoot();
    int mountDisk();
    int unmountDisk();

    void debugMessage(QString msg);
    void infoMessage(QString msg);
    static QString getBlockName(QString mmcNumber);

signals:
    void copyError(QString msg);
    void copyInfo(QString);
    void copyDebug(QString);

public:
    MMCCopyThread(QString src, QString dst);
    void run();
    static QString getInternalBlockName();
    static QString getExternalBlockName();
};



class MMCTestStart : public NovenaTest
{
public:
    MMCTestStart(QString src, QString dst);
    void runTest();
};

class MMCTestFinish : public NovenaTest
{
public:
    MMCTestFinish();
    void runTest();
};


#endif // MMCTEST_H
