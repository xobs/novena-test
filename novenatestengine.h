#ifndef NOVENATESTENGINE_H
#define NOVENATESTENGINE_H

#include <QObject>
#include <QVector>
#include <QVectorIterator>
#include "novenatest.h"
class NovenaTestWindow;
class NovenaTestEngineThread;

class NovenaTestEngine : public QObject
{
    Q_OBJECT

private:
	QList<NovenaTest *> tests;
	QList<NovenaTest *>testsToRun;

	NovenaTest *currentTest;
    int currentTestNumber;
    NovenaTestWindow *ui;
    NovenaTestEngineThread *currentThread;
	int errorCount;
	bool debugMode;
	bool serialRead;
	QString serialNumberString;
	void updateSerialNumber();

public:
    NovenaTestEngine(NovenaTestWindow *ui);
	void setDebug(bool on);
	bool loadAllTests();
	bool runAllTests();
	bool runSelectedTests(QList<NovenaTest *> &tests);
	const QList<NovenaTest *> & allTests();

	bool runNextTest(int continueOnErrors = 0);
	bool debugModeOn();
	const QString &serialNumber();

public slots:
    /* 
       @param level - 0 == info, 1 == error, 2 == debug
       @param value - An error code.  0 for success.
       @param message - An informative message to put up.
    */
    void updateTestState(const QString name, int level, int value, const QString message);
    void cleanupCurrentTest(void);

signals:
    void testsFinished();
};

#endif // NOVENATESTENGINE_H
