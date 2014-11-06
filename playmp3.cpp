#include "playmp3.h"
#include <QProcess>
#include <QThread>

class PlayMP3Thread : public QThread {
private:
    QString _path;
public:
    PlayMP3Thread(const QString &path)
    {
        _path = path;
    }

    void run()
    {
        QProcess shell;
        while(1) {
            shell.start("mpg123", QStringList() << "-o" << "alsa" << "-a" << "hw:1,0" << _path);
            shell.waitForFinished(INT_MAX);
        }
    }
};

static PlayMP3Thread *playThread;
PlayMP3::PlayMP3(const QString &path)
{
    name = "Play MP3 file";
    _path = path;
    playThread = new PlayMP3Thread(path);
}

void PlayMP3::runTest()
{
    QProcess shell;

    testInfo(QString() + "Playing MP3: " + _path);

    testDebug("Setting volume...");
    shell.start("amixer", QStringList() << "-c" << "1" << "set" << "Speaker" << "0");
    shell.waitForFinished();
    shell.start("amixer", QStringList() << "-c" << "1" << "set" << "Headphone" << "100%");
    shell.waitForFinished();
    
    testDebug("Playing MP3");
    playThread->start();
    return;
}

