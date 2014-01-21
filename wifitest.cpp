#include <unistd.h>
#include <QApplication>
#include "wifitest.h"


//#define FILE_URL "http://edmond.local/~smc/bigfile.bin"
//#define FILE_SUM "bc7ca840c077f5fcab05e1b5fc9642171a400401"
#define FILE_URL "http://192.168.100.1/bigfile.bin"
#define FILE_SUM "1bca2b65d3d4835ea4a8236c6a600a0986eea697"

WifiTest::WifiTest()
{
    name = new QString("Wifi test");
    accessManager = new QNetworkAccessManager();
    connect(accessManager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(replyFinished(QNetworkReply*)));
    connect(this, SIGNAL(doStartDownload(const char *)),
            this, SLOT(startDownload(const char *)));
}


void WifiTest::replyFinished(QNetworkReply *)
{
    finished = true;
}

void WifiTest::gotError(QNetworkReply::NetworkError)
{
    QString str("Encountered network error: ");
    str.append(reply->errorString());
    emit testStateUpdated(TEST_ERROR, 0, str);
    error = true;
    finished = true;
}

void WifiTest::downloadProgress(qint64, qint64)
{
}

void WifiTest::readyRead(void)
{
    char data[reply->bytesAvailable()];
    reply->read(data, sizeof(data));
    hash->addData(data, sizeof(data));
    file->write(data, sizeof(data));
    fileSize += sizeof(data);
}


void WifiTest::openUsbDrive()
{
    QString mountPoint;
    fileSize = 0;
    // Look through /proc/mounts for something containing "sda"

#ifdef linux
    /*
	bool found = false;

    while (!found) {
        QFile mounts("/proc/mounts");
        QString line;

        mounts.open(QIODevice::ReadOnly | QIODevice::Text);
        line = mounts.readLine();
        while (!line.isNull()) {
            if (line.contains("/media/sd")) {
                mountPoint = line.split(' ')[1];
                mountPoint.append("/testfile.bin");
                file = new QFile(mountPoint);
                if (!file->open(QIODevice::WriteOnly)) {
                    delete file;
                    file = NULL;
                    continue;
                }
                return;
            }
            line = mounts.readLine();
        }
	}
    */
#else
	file = new QFile("download-test-file.bin");
	file->open(QIODevice::WriteOnly);
#endif
    return;
}



void WifiTest::startDownload(const char *urlChar)
{
    QString urlStr(urlChar);
    QUrl url(urlStr);
    QNetworkRequest request(url);
    reply = accessManager->get(request);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
             this, SLOT(downloadProgress(qint64, qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
             this, SLOT(gotError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(readyRead()),
             this, SLOT(readyRead()));
}

void WifiTest::runTest() {

    hash = new QCryptographicHash(QCryptographicHash::Sha1);

    emit testStateUpdated(TEST_INFO, 0, QString("Looking for USB drive..."));
    openUsbDrive();
    

    emit doStartDownload(FILE_URL);
    emit testStateUpdated(TEST_INFO, 0, QString("Downloading test file..."));

    // Wait for the download to finish (in the main thread)
    finished = false;
    error = false;
    while (finished == false)
        sleep(1);
    file->close();

    if (error) {
        file->remove();
        return;
    }

    // Compare the result
    const QByteArray hashResult = hash->result().toHex();
    const QByteArray testResult(FILE_SUM);
    if (hashResult == testResult) {
        QString str = "Stream hash matches";
        emit testStateUpdated(TEST_INFO, 0, str);
    }
    else {
        QString str;
        str.sprintf("Stream hashes don't match! Got %s, wanted %s", hashResult.data(), testResult.data());
        emit testStateUpdated(TEST_ERROR, 0, str);
    }


    // Compare the result with the file on disk.
    delete hash;
    hash = new QCryptographicHash(QCryptographicHash::Sha1);
    system("sync");
    if (!file->open(QIODevice::ReadOnly)) {
        emit testStateUpdated(TEST_INFO, 0, QString("Attempted to open file for hash check but failed"));
        return;
    }

    // Read in the file
    qint64 fileOffset = 0;
    while (fileOffset < fileSize) {
        QByteArray data = file->read(4096);
        fileOffset += 4096;
        hash->addData(data);
    }
    file->close();

    // Compute the result and check it
    const QByteArray hashResult2 = hash->result().toHex();
    if (hashResult2 == testResult) {
        emit testStateUpdated(TEST_INFO, 0, QString("File hash matches"));
        file->remove();
    }
    else {
        QString str;
        str.sprintf("File hashes don't match! Got %s, wanted %s", hashResult2.data(), testResult.data());
        emit testStateUpdated(TEST_ERROR, 0, str);
    }
    delete hash;
    hash = NULL;


    return;
}
