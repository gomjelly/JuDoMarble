#pragma once
#include <QWidget>
#include <QTransform>
#include <QVector>
#include "PolygonItem.h"
#include "BuildingDialog.h"
#include "GameState.h"

class QScreen;
class QTimer;
class EditorCanvas;

// ── Firework particle ──────────────────────────────────────────────────────────
struct Particle {
    QPointF pos;        // board coords
    QPointF vel;        // board coords/s
    QColor  color;
    float   life;       // remaining seconds
    float   maxLife;
};

// ── Intro phase ────────────────────────────────────────────────────────────────
enum class IntroPhase {
    GameStart,          // "게임 시작!" splash
    TurnAnnounce,       // "순서를 정합니다!"
    TurnReveal,         // revealing each player's order
    FestivalAnnounce,   // "축제의 도시를 선정합니다!"
    FestivalReveal,     // revealing each festival city
    Playing             // normal gameplay
};

class SimulationWindow : public QWidget {
    Q_OBJECT
public:
    SimulationWindow(EditorCanvas* canvas, QScreen* screen,
                     GameState* gameState = nullptr,
                     QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private slots:
    void onAnimTick();
    void onIntroStep();

private:
    EditorCanvas*  m_canvas;
    GameState*     m_gameState{ nullptr };

    // Animation
    QTimer*  m_animTimer{ nullptr };
    float    m_animPhase{ 0.f };   // 0..2π, loops
    QVector<Particle> m_particles;

    // Intro sequence
    IntroPhase m_introPhase{ IntroPhase::GameStart };
    int        m_revealCount{ 0 }; // how many items revealed so far in current phase

    // Board transform (set in paintEvent, used by mouse hit tests)
    QTransform m_boardTransform;
    bool       m_boardTransformValid{ false };

    // Token dragging
    int   m_draggingPlayer{ -1 };
    QPointF m_dragOffset;   // offset from token center in screen coords

    void startIntroSequence();
    void advanceIntro();
    void scheduleIntroStep(int ms);

    // Drawing helpers
    void drawGame(QPainter& p, const QTransform& boardT);
    void drawFestivalGlow(QPainter& p, int cellIdx, float phase);
    void drawPlayerToken(QPainter& p, int playerIdx, bool isActive, float phase);
    void drawTurnBanner(QPainter& p);
    void drawIntroOverlay(QPainter& p);
    void drawParticles(QPainter& p);
    void spawnFireworks(QPointF boardCenter, QColor color, int count = 18);

    // Helpers
    QPointF tokenCenter(int playerIdx) const;     // screen coords
    QPointF boardCellCenter(int cellIdx) const;   // board coords
    int     nearestBoardCell(QPointF boardPos) const;
    QPointF screenToBoard(QPointF screen) const;
};
