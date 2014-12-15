#include "destructivedisktest.h"

#include <QFile>
#include <QCryptographicHash>

#define BUFFER_SIZE 512

DestructiveDiskTest::DestructiveDiskTest(quint32 _bytes, const QString _path, const QString _type) : path(_path)
{
    name = QString("Destructive Disk Test: %1").arg(_type);
    bytes = _bytes;
}

void DestructiveDiskTest::runTest()
{
    QFile readFile(path);
    QFile writeFile(path);
    QCryptographicHash readHash(QCryptographicHash::Sha1);
    QCryptographicHash writeHash(QCryptographicHash::Sha1);

    if (!writeFile.open(QIODevice::WriteOnly)) {
        testError(QString("Unable to open disk for writing: %1").arg(writeFile.errorString()));
        return;
    }

    for (quint32 counter = 0; counter < bytes; counter += BUFFER_SIZE) {
        QByteArray buf(BUFFER_SIZE, counter);
        writeHash.addData(buf);
        writeFile.write(buf);
    }

    testInfo(QString("Write data hash: ")+ writeHash.result().toHex());
    writeFile.close();

    if (!readFile.open(QIODevice::ReadOnly)) {
        testError(QString("Unable to open disk for reading: %1").arg(readFile.errorString()));
        return;
    }

    for (quint32 counter = 0; counter < bytes; counter += BUFFER_SIZE) {
        QByteArray buf = readFile.read(BUFFER_SIZE);
        readHash.addData(buf);
    }

    testInfo(QString("Read data hash: ") + readHash.result().toHex());
    readFile.close();

    if (!(readHash.result() == writeHash.result()))
        testError("Hashes do not match");
    else
        testInfo("Hashes match");
}
