#ifndef KOVANTESTWINDOW_H
#define KOVANTESTWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "kovantestengine.h"

const char SEQUENCE[] = "132231";

namespace Ui {
class KovanTestWindow;
}

class KovanTestWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit KovanTestWindow(QWidget *parent = 0);
    ~KovanTestWindow();
    void setStatusText(QString *message);
    void setProgressBar(double progress);
    void setProgressText(QString &text);
	void setErrorString(QString &message);
    void addTestLog(QString &message);
	void finishTests(bool successful);

private:
    Ui::KovanTestWindow *ui;
    KovanTestEngine *engine;
	QString errorString;
	unsigned int sequencePosition;
	void advanceDebugSequence(const char c);
	QString serialLabelString;

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

#endif // KOVANTESTWINDOW_H
