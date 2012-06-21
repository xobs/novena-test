#ifndef WIFITEST_H
#define WIFITEST_H

#include "kovantest.h"
#include <Qt/QtNetwork>

class WifiTest : public KovanTest
{
    Q_OBJECT

    bool finished;
    QNetworkReply *reply;
    QNetworkAccessManager *accessManager;
    QCryptographicHash *hash;
    QFile *file;
public:
    WifiTest();
    void runTest();
    void openUsbDrive();

public slots:
    void replyFinished(QNetworkReply *reply);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void readyRead(void);
    void startDownload(const char *url);

signals:
    void doStartDownload(const char *url);
};

#endif // WIFITEST_H
