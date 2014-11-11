#include "audiotest.h"

#include <QAudioOutput>
#include <QAudioFormat>

class PleasantSound : public QIODevice {

private:
    double _rate;
    int _nchan;
    int _buffersize;

#define TONES_MAX 10
    double pl_note;
    double pl_lastnote;
    double pl_a, pl_b, pl_d;
    double pl_s; /* Current sample buffer */
    double pl_ca, pl_cb;
    double pl_de, pl_di;
    int loop_count;

    double tones[TONES_MAX];

    enum state {
        state_new,
        state_drain,

        /* Anti-pop, return value to center */
        state_clt,
        state_cgt,
    };
    enum state state;

public:
    PleasantSound(int rate, int nchan, int buffersize) {
        _rate = rate;
        _nchan = nchan;
        _buffersize = buffersize;
        _rate = rate;
        pl_s = 128;
        state = state_new;
        loop_count = 0;
#if 0
        double pl_scale;
        int i;
        pl_scale = pow(0.5, 1.0/5); /* Fifth root, pentatonic */
        for (i = 0; i < TONES_MAX; i++)
                tones[i] = (_rate / 160.0) * pow(pl_scale, i);
#endif
#if 0
        double pl_scale;
        pl_scale = pow(0.5, 1.0/12); /* Fifth root, pentatonic */
        tones[0] = (_rate / 160.0) * pow(pl_scale, 0);
        tones[1] = (_rate / 160.0) * pow(pl_scale, 1);
        tones[2] = (_rate / 160.0) * pow(pl_scale, 2);
        tones[3] = (_rate / 160.0) * pow(pl_scale, 4);
        tones[4] = (_rate / 160.0) * pow(pl_scale, 5);
        tones[5] = (_rate / 160.0) * pow(pl_scale, 7);
        tones[6] = (_rate / 160.0) * pow(pl_scale, 8);
        tones[7] = (_rate / 160.0) * pow(pl_scale, 9);
        tones[8] = (_rate / 160.0) * pow(pl_scale, 11);
        tones[9] = (_rate / 160.0) * pow(pl_scale, 12);
#endif
        tones[0] = 56;
        tones[1] = 64;
        tones[2] = 72;
        tones[3] = 81;
        tones[4] = 96;
        tones[5] = tones[0] * 2;
        tones[6] = tones[1] * 2;
        tones[7] = tones[2] * 2;
        tones[8] = tones[3] * 2;
        tones[9] = tones[4] * 2;

        pl_note = pleasant_wl();
    }

    ~PleasantSound() {}

    qint64 readData(char *data, qint64 maxSize) {
        if (!maxSize)
            return 0;
        pleasant_gen((quint16 *)data, maxSize / (_nchan * 2));
        return maxSize;
    }

    qint64 writeData(const char *, qint64) {
        return -1;
    }

    double pl_rand(void) {
            return ((double)rand()) / RAND_MAX;
    }

    double pleasant_wl(void) {
        int index = pl_rand() * TONES_MAX;
        double val = tones[index];
        //static int index = 0;
        //double val = tones[index++];
        if (index >= TONES_MAX)
            index = 0;
        return val;
    }

    void pleasant_putsample(quint16 *bfr, int offset, double s) {
        int chan;

        for (chan = 0; chan < _nchan; chan++)
            bfr[offset * _nchan + chan] = (s - 128) * (256.0);
    }

    int pleasant_gen(quint16 *bfr, int nsamples) {
        int off;

        for (off = 0; off < nsamples; off++) {

            switch (state) {

            case state_new:
                pl_lastnote = pl_note;
                pl_note = pleasant_wl();

                /* Avoid picking a new note that's the same as the
                 * last note.
                 */
                if (pl_note == pl_lastnote)
                    pl_note *= 2;

                pl_d = (pl_rand() * 10.0 + 5.0) * _rate / 4.0;
                pl_a = 0;
                pl_b = 0;
                pl_s = 128;
                pl_ca = 40 / pl_note;
                pl_cb = 20 / pl_lastnote;
                pl_de = _rate / 10;
                pl_di = 0;

                state = state_drain;
                loop_count = 0;
                /* Fallthrough */

            case state_drain:
                pl_a++;
                pl_b++;
                pl_di++;
                pl_s = pl_s + (pl_ca + pl_cb);
                if (pl_a > pl_note) {
                    pl_a = 0;
                    pl_ca *= -1;
                }
                if (pl_b > pl_lastnote) {
                    pl_b = 0;
                    pl_cb *= -1;
                }
                if (pl_di > pl_de) {
                    pl_di = 0;
                    pl_ca *= 0.9;
                    pl_cb *= 0.9;
                }
                pleasant_putsample(bfr, off, pl_s);

                loop_count++;
                if (loop_count >= pl_d) {
                    pl_s = (int)pl_s;
                    if (pl_s < 128)
                        state = state_clt;
                    else
                        state = state_cgt;
                }
                break;

            case state_clt:
                pleasant_putsample(bfr, off, pl_s);
                pl_s++;

                if (pl_s >= 128)
                    state = state_new;
                break;

            case state_cgt:
                pleasant_putsample(bfr, off, pl_s);
                pl_s--;

                if (pl_s <= 128)
                    state = state_new;
                break;

            default:
                return -1;
            }
        }
        return 0;
    }
};

QAudioOutput *AudioTest::openAudioDevice(const QAudioDeviceInfo &info, const QAudioFormat &format) {
    QAudioFormat f = format;

    if (!info.isFormatSupported(f))
        f = info.nearestFormat(f);

    return new QAudioOutput(info, f, NULL);
}

AudioTest::AudioTest()
{
    name = "Audio";

    QAudioFormat format;

    // Set up the format, eg.
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    output = NULL;
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo.deviceName() == "alsa_output.platform-sound.analog-stereo")
            output = openAudioDevice(deviceInfo, format);

        if (output)
            break;
    }

    /* No matching audio device found, open the default one */
    if (!output)
        output = openAudioDevice(QAudioDeviceInfo::defaultOutputDevice(), format);
}

void AudioTest::runTest() {

    if (!output) {
        testError("Couldn't open audio device");
        return;
    }

    testInfo("Producing pleasant audio sounds");
    input = new PleasantSound(22050, 2, 1024);
    input->open(QIODevice::ReadOnly);

    output->setBufferSize(524288);
    output->start(input);
    output->setVolume(0.8);
}
