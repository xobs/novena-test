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
    QString bootloader;
    QFile outputImage;
    QFile inputImage;
    QFile bootloaderImage;

    int findDevices();
    int partitionDevice();
    int formatDevice();
    int loadBootloader();
    int mountDevice();
    int extractImage();
    int unmountDevice();

    void debugMessage(QString msg);
    void infoMessage(QString msg);
    static QString getBlockName(QString mmcNumber);

signals:
    void copyError(QString msg);
    void copyInfo(QString);
    void copyDebug(QString);

public:
    MMCCopyThread(QString src, QString bl, QString dst);
    void run();
    static QString getInternalBlockName();
    static QString getExternalBlockName();
};



class MMCTestStart : public NovenaTest
{
public:
    MMCTestStart(QString src, QString bl, QString dst);
    void runTest();
};

class MMCTestFinish : public NovenaTest
{
public:
    MMCTestFinish();
    void runTest();
};


#endif // MMCTEST_H
