#pragma once
#include <QObject>

class QAudioSink;
class QBuffer;

class SoundManager : public QObject {
    Q_OBJECT
public:
    static SoundManager& instance();

    void playDialogOpen();
    void playBuildingPlaced();
    void playSimulationStart();

private:
    explicit SoundManager(QObject* parent = nullptr);

    // Generates 16-bit mono PCM sine tone with linear attack/decay envelope
    static QByteArray makeTone(float freqHz, float durSec, float volume,
                               float attackSec, float decaySec, int sampleRate = 44100);

    void play(const QByteArray& pcm, int sampleRate = 44100);
};
