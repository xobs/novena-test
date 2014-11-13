#include "audiotest.h"

#include <QAudioDeviceInfo>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QSound>

AudioTest::AudioTest() : addSoundsTimer(this)
{
    name = "Audio";

    QDir soundDir("/factory/files/sounds/");
    foreach (QFileInfo fileInfo, soundDir.entryInfoList()) {
        if (fileInfo.fileName().endsWith("wav")) {
            QSoundEffect *newEffect = new QSoundEffect();
            newEffect->setSource(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
            qDebug() << "Loading sound" << fileInfo.absoluteFilePath();
            sounds.append(newEffect);
        }
    }
    maxSounds = 5;
    addSoundsTimer.setSingleShot(false);
    addSoundsTimer.setInterval(300);
    connect(this, SIGNAL(startTimer()), &addSoundsTimer, SLOT(start()));
}

void AudioTest::runTest()
{
    testInfo("Producing pleasant audio sounds");

    /* Hacks */
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
    QMutableListIterator<QSoundEffect *> iterator(playing);
    while (iterator.hasNext()) {
        QSoundEffect *fx = iterator.next();
        if (!fx->isPlaying()) {
            qDebug() << "Finished:" << fx->source();
            iterator.remove();
        }
    }

    /* Only play a sound when we're below the maximum number of sounds, and
     * about 70% of the time.
     */
    if ((playing.length() < maxSounds) && (randd() > 0.7)) {
        QSoundEffect *fx = NULL;

        /* Try a few times to get a new sound to play, one that's not already playing */
        for (int i = 0; i < 5; i++) {
            QSoundEffect *newfx = sounds.at((int)(randd() * sounds.length()));
            if (!playing.contains(newfx)) {
                fx = newfx;
                break;
            }
        }

        if (fx) {
            qDebug() << "Picked to start:" << fx->source();
            fx->play();
            playing.append(fx);
        }
    }
    mutex.unlock();
}
