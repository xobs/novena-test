#ifndef KOVANTESTENGINE_H
#define KOVANTESTENGINE_H

#include <QObject>
#include <QVector>
#include <QVectorIterator>
#include "kovantest.h"
class KovanTestWindow;

class KovanTestEngine : public QObject
{
    Q_OBJECT

private:
    QVector<KovanTest *> tests;
    KovanTest *currentTest;
    int currentTestNumber;
    KovanTestWindow *ui;

public:
    KovanTestEngine(KovanTestWindow *ui);
    bool loadAllTests();
    bool runAllTests();

    bool runNextTest();

public slots:
    /* @param running - Determines whether the test is still running (0 if it's done)
       @param value - An error code.  0 for success.
       @param message - An informative message to put up.
    */
    void updateTestState(int running, int value, QString *message);
};

#endif // KOVANTESTENGINE_H
