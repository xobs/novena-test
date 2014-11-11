#ifndef AUDIOTEST_H
#define AUDIOTEST_H

#include "novenatest.h"

class QAudioOutput;
class PleasantSound;
class QAudioDeviceInfo;
class QAudioFormat;

class AudioTest : public NovenaTest
{
	Q_OBJECT

    QAudioOutput *output;
    PleasantSound *input;
    QAudioOutput *openAudioDevice(const QAudioDeviceInfo &info, const QAudioFormat &format);

public:
    AudioTest();
    void runTest();

//public slots:
//    void handleStateChanged(QAudio::State newState);
};

#endif // AUDIOTEST_H
