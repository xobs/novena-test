#ifndef AUDIOTEST_H
#define AUDIOTEST_H

#include <QSound>
#include <QList>
#include <QMutex>
#include <QTimer>
#include "novenatest.h"

class AudioTest : public NovenaTest
{
	Q_OBJECT

    QList<QSound *> sounds;
    QList<QSound *> playing;
    int maxSounds;
    QMutex mutex;
    QTimer addSoundsTimer;

public:
    AudioTest();
    void runTest();

public slots:
//    void soundFinished(void);
//    void statusChanged(void);
    void timerFinished(void);
    qreal randd(void);

signals:
    void startTimer();
};

class AudioTestStop : public NovenaTest
{
    Q_OBJECT

public:
    AudioTestStop();
    void runTest();
};

#endif // AUDIOTEST_H
