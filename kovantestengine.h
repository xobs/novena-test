#ifndef KOVANTESTENGINE_H
#define KOVANTESTENGINE_H

#include <QObject>
#include <QVector>
#include <QVectorIterator>
#include "kovantest.h"
class KovanTestWindow;
class KovanTestEngineThread;

class KovanTestEngine : public QObject
{
    Q_OBJECT

private:
	QList<KovanTest *> tests;
	QList<KovanTest *>testsToRun;

	KovanTest *currentTest;
    int currentTestNumber;
    KovanTestWindow *ui;
    KovanTestEngineThread *currentThread;
	int errorCount;
	bool debugMode;
	QString serialNumberString;

public:
    KovanTestEngine(KovanTestWindow *ui);
	void setDebug(bool on);
	bool loadAllTests();
	bool runAllTests();
	bool runSelectedTests(QList<KovanTest *> &tests);
	const QList<KovanTest *> & allTests();

	bool runNextTest(int continueOnErrors = 0);
	bool debugModeOn();
	const QString &serialNumber();

public slots:
    /* 
       @param level - 0 == info, 1 == error, 2 == debug
       @param value - An error code.  0 for success.
       @param message - An informative message to put up.
    */
    void updateTestState(int level, int value, QString *message);
    void cleanupCurrentTest(void);
};

#endif // KOVANTESTENGINE_H
