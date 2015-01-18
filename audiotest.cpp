#include "audiotest.h"

#include <QAudioDeviceInfo>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <unistd.h>

static bool endTest;

AudioTestStop::AudioTestStop()
{
    name = "Stop audio test";
}

void AudioTestStop::runTest(void)
{
    testInfo("Stopping audio test");
    endTest = true;
}

AudioTest::AudioTest() : addSoundsTimer(this)
{
    name = "Audio";

    QDir soundDir("/factory/sounds/");
    foreach (QFileInfo fileInfo, soundDir.entryInfoList()) {
        if (fileInfo.fileName().endsWith("wav")) {
            QSound *newSound = new QSound(fileInfo.absoluteFilePath());
            qDebug() << "Loading sound" << fileInfo.absoluteFilePath();
            sounds.append(newSound);
        }
    }
    maxSounds = 5;
    addSoundsTimer.setSingleShot(false);
    addSoundsTimer.setInterval(400);
    connect(this, SIGNAL(startTimer()), &addSoundsTimer, SLOT(start()));
}

void AudioTest::runTest()
{
    testInfo("Producing pleasant audio sounds");
    endTest = false;

    /* Hacks */
    system("cat /factory/sounds/* > /dev/null");
    system("amixer -c 1 set 'Right Mixer Right Bypass' unmute");
    system("amixer -c 1 set 'Left Mixer Left Bypass' unmute");
    system("amixer -c 1 set 'Output 1' 100%"); /* Headphone port 100% */
    system("amixer -c 1 set 'Output 2' 100%");
    system("amixer set Master 100%");

    connect(&addSoundsTimer, SIGNAL(timeout()), this, SLOT(timerFinished()));
    emit startTimer();
}

qreal AudioTest::randd(void)
{
    return ((qreal)rand()) / ((qreal)RAND_MAX);
}

void AudioTest::timerFinished(void)
{
    mutex.lock();

    /* Remove any not-playing sounds */
    QMutableListIterator<QSound *> iterator(playing);
    while (iterator.hasNext()) {
        QSound *fx = iterator.next();
        if (fx->isFinished()) {
            qDebug() << "Finished:" << fx->fileName();
            iterator.remove();
        }
    }

    if (endTest)
        goto out;

    /* Only play a sound when we're below the maximum number of sounds, and
     * about 70% of the time.
     */
    if ((playing.length() < maxSounds)) {
        qreal chance = randd();
        qDebug() << "Chance of playing sound:" << chance;
        if (chance > 0.3) {
            usleep(randd() * 200000);
            QSound *fx = NULL;

            /* Try a few times to get a new sound to play, one that's not already playing */
            for (int i = 0; i < 5; i++) {
                int idx = randd() * sounds.length();
                QSound *newfx = sounds.at(idx);
                if (!playing.contains(newfx)) {
                    fx = newfx;
                    break;
                }
            }

            if (fx) {
                qDebug() << "Picked to start:" << fx->fileName();
                fx->play();
                playing.append(fx);
            }
        }
    }

out:
    mutex.unlock();
}
