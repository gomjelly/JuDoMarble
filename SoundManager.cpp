#include "SoundManager.h"
#include <QAudioSink>
#include <QAudioFormat>
#include <QBuffer>
#include <QMediaDevices>
#include <cmath>

SoundManager& SoundManager::instance() {
    static SoundManager inst;
    return inst;
}

SoundManager::SoundManager(QObject* parent) : QObject(parent) {}

// ── PCM generation ─────────────────────────────────────────────────────────────
QByteArray SoundManager::makeTone(float freq, float dur, float vol,
                                   float attack, float decay, int sr)
{
    int n = (int)(dur * sr);
    QByteArray data(n * (int)sizeof(int16_t), 0);
    auto* s = reinterpret_cast<int16_t*>(data.data());
    for (int i = 0; i < n; ++i) {
        float t   = (float)i / sr;
        float env;
        if      (t < attack)        env = t / attack;
        else if (t > dur - decay)   env = (dur - t) / decay;
        else                        env = 1.0f;
        env = env * env; // smooth curve
        s[i] = (int16_t)(vol * env * std::sin(2.0f * (float)M_PI * freq * t) * 32767.0f);
    }
    return data;
}

// ── Playback ───────────────────────────────────────────────────────────────────
void SoundManager::play(const QByteArray& pcm, int sampleRate) {
    QAudioFormat fmt;
    fmt.setSampleRate(sampleRate);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    auto* buf  = new QBuffer(this);
    buf->setData(pcm);
    buf->open(QIODevice::ReadOnly);

    auto* sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), fmt, this);

    // Auto-cleanup when playback finishes
    connect(sink, &QAudioSink::stateChanged, this, [sink, buf](QAudio::State state) {
        if (state == QAudio::IdleState) {
            sink->deleteLater();
            buf->deleteLater();
        }
    });

    sink->start(buf);
}

// ── Sound effects ──────────────────────────────────────────────────────────────

void SoundManager::playDialogOpen() {
    // 부드러운 벨: A5 (880Hz), 0.12초
    play(makeTone(880.0f, 0.12f, 0.35f, 0.004f, 0.09f));
}

void SoundManager::playBuildingPlaced() {
    // 상승하는 두 음: G4 → C5
    QByteArray pcm;
    pcm += makeTone(392.0f,  0.08f, 0.5f, 0.004f, 0.04f);
    pcm += makeTone(523.25f, 0.15f, 0.5f, 0.004f, 0.11f);
    play(pcm);
}

void SoundManager::playSimulationStart() {
    // 팡파레: C5 → E5 → G5 → C6 (상승 4음)
    QByteArray pcm;
    pcm += makeTone( 523.25f, 0.10f, 0.6f, 0.004f, 0.05f);
    pcm += makeTone( 659.25f, 0.10f, 0.6f, 0.004f, 0.05f);
    pcm += makeTone( 783.99f, 0.10f, 0.6f, 0.004f, 0.05f);
    pcm += makeTone(1046.50f, 0.32f, 0.6f, 0.004f, 0.22f);
    play(pcm);
}

void SoundManager::playGameStart() {
    // 화려한 게임 시작 팡파레: 빠른 상승 아르페지오 + 긴 마무리
    QByteArray pcm;
    pcm += makeTone( 392.00f, 0.07f, 0.5f, 0.002f, 0.03f);
    pcm += makeTone( 523.25f, 0.07f, 0.5f, 0.002f, 0.03f);
    pcm += makeTone( 659.25f, 0.07f, 0.5f, 0.002f, 0.03f);
    pcm += makeTone( 783.99f, 0.07f, 0.5f, 0.002f, 0.03f);
    pcm += makeTone( 987.77f, 0.07f, 0.6f, 0.002f, 0.03f);
    pcm += makeTone(1046.50f, 0.07f, 0.7f, 0.002f, 0.03f);
    pcm += makeTone(1318.51f, 0.45f, 0.7f, 0.004f, 0.35f);
    play(pcm);
}

void SoundManager::playRevealItem() {
    // 짧은 "핑" - 아이템 공개
    QByteArray pcm;
    pcm += makeTone(880.00f, 0.06f, 0.4f, 0.002f, 0.04f);
    pcm += makeTone(1174.66f, 0.18f, 0.5f, 0.002f, 0.15f);
    play(pcm);
}

void SoundManager::playFestivalReveal() {
    // 축제 도시 선정: 밝고 경쾌한 멜로디
    QByteArray pcm;
    pcm += makeTone( 659.25f, 0.08f, 0.55f, 0.003f, 0.04f);
    pcm += makeTone( 783.99f, 0.08f, 0.55f, 0.003f, 0.04f);
    pcm += makeTone( 987.77f, 0.08f, 0.55f, 0.003f, 0.04f);
    pcm += makeTone(1318.51f, 0.08f, 0.60f, 0.003f, 0.04f);
    pcm += makeTone(1567.98f, 0.35f, 0.65f, 0.003f, 0.28f);
    play(pcm);
}

void SoundManager::playPlayerTurn() {
    // 플레이어 차례 알림: 경쾌한 2음
    QByteArray pcm;
    pcm += makeTone(783.99f, 0.10f, 0.5f, 0.003f, 0.05f);
    pcm += makeTone(987.77f, 0.22f, 0.6f, 0.003f, 0.16f);
    play(pcm);
}
