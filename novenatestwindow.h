#ifndef NOVENATESTWINDOW_H
#define NOVENATESTWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QFile>
#include "novenatestengine.h"

const char SEQUENCE[] = "132231";

namespace Ui {
class NovenaTestWindow;
}

class NovenaTestWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit NovenaTestWindow(QWidget *parent = 0);
    ~NovenaTestWindow();
    void setStatusText(QString *message);
    void setProgressBar(double progress);
    void setProgressText(QString &text);
	void setErrorString(QString &message);
    void addTestLog(QString &message);
	void finishTests(bool successful);

private:
    Ui::NovenaTestWindow *ui;
    NovenaTestEngine *engine;
	QString errorString;
	unsigned int sequencePosition;
	QString serialLabelString;
	QFile logFile;

	void advanceDebugSequence(const char c);
	void openLogFile();

public slots:
    void startTests();
	void debugMode1Clicked();
	void debugMode2Clicked();
	void debugMode3Clicked();
	void debugItemPressed(QListWidgetItem *item);
	void debugRunSelectedItems();

	void moveToDebugScreen();
	void moveToMainScreen();
};

#endif // NOVENATESTWINDOW_H
