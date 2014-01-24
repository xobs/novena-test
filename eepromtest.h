#ifndef EEPROMTEST_H
#define EEPROMTEST_H
#include <QThread>
#include <QFile>
#include "novenatest.h"

class EEPROMStart : public NovenaTest
{
private:
    QString _url;
    QString _features;

public:
    EEPROMStart(QString url, QString features);
    void runTest();
};

class EEPROMFinish : public NovenaTest
{
private:
    QString _url;

public:
    EEPROMFinish(QString url);
    void runTest();
};


#endif // EEPROMTEST_H
