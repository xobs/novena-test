#ifndef NOVENATEST_H
#define NOVENATEST_H

#include <QString>
#include <QObject>

class NovenaTestEngine;

#define TEST_ERROR 1
#define TEST_INFO 0
#define TEST_DEBUG 2

class NovenaTest : public QObject
{
    Q_OBJECT

private:
    int testNumber;
    QString *lastString;
    int lastResult;
    NovenaTestEngine *engine;

protected:
    QString name;

public:
    NovenaTest();
    virtual void runTest() = 0;

    /* Called by the engine after a test has finished running */
    QString *getStatusString();
    int getStatusValue();

    /* Used for issuing callbacks */
    void setEngine(NovenaTestEngine *engie);

    /* Tells this test what its position is */
    void setTestNumber(int number);
    const QString testName();

public slots:
    void testInfo(const QString string);
    void testError(const QString string);
    void testDebug(const QString string);

signals:
    void testStateUpdated(const QString name, int level, int value, QString message);
};

#endif // NOVENATEST_H
