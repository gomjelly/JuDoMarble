#include "SimulationWindow.h"
#include "EditorCanvas.h"
#include "BoardData.h"
#include "SoundManager.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QtMath>
#include <QRandomGenerator>

static constexpr float  kAnimSpeed  = 2.0f;   // radians/s
static constexpr int    kAnimMs     = 40;      // ~25fps tick
static constexpr float  kTokenR     = 14.0f;  // normal token radius in board coords
static constexpr float  kTokenRact  = 20.0f;  // active token radius

// ── Constructor ───────────────────────────────────────────────────────────────
SimulationWindow::SimulationWindow(EditorCanvas* canvas, QScreen* screen,
                                   GameState* gameState, QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
    , m_canvas(canvas)
    , m_gameState(gameState)
{
    setWindowTitle("JuDoMarble");
    setGeometry(screen->geometry());
    show();
    windowHandle()->setScreen(screen);
    showFullScreen();

    connect(m_canvas, &EditorCanvas::boardChanged, this, QOverload<>::of(&QWidget::update));

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &SimulationWindow::onAnimTick);
    m_animTimer->start(kAnimMs);

    if (m_gameState && m_gameState->isActive)
        startIntroSequence();
}

// ── Animation tick ────────────────────────────────────────────────────────────
void SimulationWindow::onAnimTick() {
    float dt = kAnimMs / 1000.0f;
    m_animPhase = std::fmod(m_animPhase + kAnimSpeed * dt, 2.0f * (float)M_PI);

    // Update particles
    for (auto& part : m_particles) {
        part.pos   += part.vel * dt;
        part.vel   *= 0.92f;             // friction
        part.vel.ry() += 40.0f * dt;     // gravity in board coords
        part.life  -= dt;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p){ return p.life <= 0; }),
        m_particles.end());

    // Continuously spawn fireworks on festival cities in playing mode
    if (m_gameState && m_gameState->isActive &&
        m_introPhase == IntroPhase::Playing &&
        !m_gameState->festivalCities.isEmpty())
    {
        static int spawnCounter = 0;
        ++spawnCounter;
        if (spawnCounter % 18 == 0) { // every ~18 ticks ≈ 0.7s
            int ridx = QRandomGenerator::global()->bounded(m_gameState->festivalCities.size());
            int cidx = m_gameState->festivalCities[ridx];
            QPointF center = boardCellCenter(cidx);
            QColor  col    = QColor::fromHsvF(
                (float)QRandomGenerator::global()->generateDouble(), 0.9f, 1.0f);
            spawnFireworks(center, col, 10);
        }
    }

    update();
}

// ── Intro sequence ─────────────────────────────────────────────────────────────
void SimulationWindow::startIntroSequence() {
    m_introPhase  = IntroPhase::GameStart;
    m_revealCount = 0;
    SoundManager::instance().playGameStart();
    scheduleIntroStep(1800);
}

void SimulationWindow::scheduleIntroStep(int ms) {
    QTimer::singleShot(ms, this, &SimulationWindow::onIntroStep);
}

void SimulationWindow::onIntroStep() {
    advanceIntro();
    update();
}

void SimulationWindow::advanceIntro() {
    if (!m_gameState) return;

    switch (m_introPhase) {
    case IntroPhase::GameStart:
        m_introPhase  = IntroPhase::TurnAnnounce;
        m_revealCount = 0;
        SoundManager::instance().playRevealItem();
        scheduleIntroStep(1200);
        break;

    case IntroPhase::TurnAnnounce:
        m_introPhase  = IntroPhase::TurnReveal;
        m_revealCount = 0;
        scheduleIntroStep(900);
        break;

    case IntroPhase::TurnReveal:
        ++m_revealCount;
        SoundManager::instance().playRevealItem();
        if (m_revealCount >= m_gameState->turnOrder.size()) {
            m_introPhase  = IntroPhase::FestivalAnnounce;
            m_revealCount = 0;
            scheduleIntroStep(1400);
        } else {
            scheduleIntroStep(900);
        }
        break;

    case IntroPhase::FestivalAnnounce:
        m_introPhase  = IntroPhase::FestivalReveal;
        m_revealCount = 0;
        SoundManager::instance().playFestivalReveal();
        scheduleIntroStep(1000);
        break;

    case IntroPhase::FestivalReveal: {
        ++m_revealCount;
        SoundManager::instance().playRevealItem();
        // Spawn fireworks on revealed city
        if (m_revealCount <= m_gameState->festivalCities.size()) {
            int cidx = m_gameState->festivalCities[m_revealCount - 1];
            spawnFireworks(boardCellCenter(cidx), QColor(255, 220, 50), 25);
        }
        if (m_revealCount >= m_gameState->festivalCities.size()) {
            m_introPhase = IntroPhase::Playing;
            SoundManager::instance().playPlayerTurn();
            // No more scheduling — game loop begins
        } else {
            scheduleIntroStep(1000);
        }
        break;
    }

    case IntroPhase::Playing:
        break;
    }
}

// ── Board transform helper ────────────────────────────────────────────────────
static QTransform computeBoardTransform(EditorCanvas* canvas, int w, int h) {
    if (!canvas->hasBoard()) return {};
    QTransform canvasToScreen = QTransform::fromScale(
        (qreal)w / EditorCanvas::CANVAS_W,
        (qreal)h / EditorCanvas::CANVAS_H);
    QPolygonF src, dst;
    src << QPointF(0,0) << QPointF(kBoardSize,0)
        << QPointF(kBoardSize,kBoardSize) << QPointF(0,kBoardSize);
    const QPointF* corners = canvas->boardCorners();
    for (int ci = 0; ci < 4; ++ci)
        dst << canvasToScreen.map(corners[ci]);
    QTransform boardT;
    if (!QTransform::quadToQuad(src, dst, boardT)) return {};
    return boardT;
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void SimulationWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);

    QTransform canvasToScreen = QTransform::fromScale(
        (qreal)width()  / EditorCanvas::CANVAS_W,
        (qreal)height() / EditorCanvas::CANVAS_H);

    if (m_canvas->hasBoard()) {
        QPolygonF src, dst;
        src << QPointF(0,0) << QPointF(kBoardSize,0)
            << QPointF(kBoardSize,kBoardSize) << QPointF(0,kBoardSize);
        const QPointF* corners = m_canvas->boardCorners();
        for (int ci = 0; ci < 4; ++ci)
            dst << canvasToScreen.map(corners[ci]);

        QTransform boardT;
        if (QTransform::quadToQuad(src, dst, boardT)) {
            m_boardTransform      = boardT;
            m_boardTransformValid = true;

            p.save();
            p.setTransform(boardT);
            EditorCanvas::drawBoard(p, m_canvas->cellStates(), m_canvas->allDownMode());
            EditorCanvas::drawBuildings(p, m_canvas->cellStates(), m_canvas->allDownMode());
            p.restore();

            if (m_gameState && m_gameState->isActive)
                drawGame(p, boardT);
        }
    }

    // Draw non-board polygon items
    for (const auto& item : m_canvas->items()) {
        QPolygonF poly = canvasToScreen.map(item.polygon);
        p.setBrush(item.color);
        p.setPen(QPen(Qt::white, 2));
        p.drawPolygon(poly);
    }

    if (m_gameState && m_gameState->isActive)
        drawIntroOverlay(p);
}

// ── Game drawing ──────────────────────────────────────────────────────────────
void SimulationWindow::drawGame(QPainter& p, const QTransform& boardT) {
    if (!m_gameState) return;

    // Festival city glows
    int festivalVisible = 0;
    if      (m_introPhase == IntroPhase::Playing)         festivalVisible = m_gameState->festivalCities.size();
    else if (m_introPhase == IntroPhase::FestivalReveal)  festivalVisible = m_revealCount;

    for (int i = 0; i < festivalVisible && i < m_gameState->festivalCities.size(); ++i) {
        p.save();
        p.setTransform(boardT);
        drawFestivalGlow(p, m_gameState->festivalCities[i], m_animPhase);
        p.restore();
    }

    // Particles (in screen coords already after spawnFireworks used board->screen)
    drawParticles(p);

    // Player tokens
    int curPlayer = (m_introPhase == IntroPhase::Playing)
                    ? m_gameState->currentPlayerIndex() : -1;
    for (int i = 0; i < m_gameState->players.size(); ++i) {
        p.save();
        p.setTransform(boardT);
        drawPlayerToken(p, i, (i == curPlayer), m_animPhase);
        p.restore();
    }

    // Turn banner (screen-space overlay)
    if (m_introPhase == IntroPhase::Playing)
        drawTurnBanner(p);
}

// ── Festival city glow ────────────────────────────────────────────────────────
void SimulationWindow::drawFestivalGlow(QPainter& p, int cellIdx, float phase) {
    QRectF r = boardSpaceRect(cellIdx);
    float  pulse = 0.5f + 0.5f * std::sin(phase * 2.5f);

    // Pulsing colored border rings
    for (int ring = 3; ring >= 1; --ring) {
        float expand = ring * 4.0f;
        float alpha  = pulse * (0.35f / ring);
        QColor c(255, 210, 30, (int)(alpha * 255));
        p.setPen(QPen(c, 3.0f + ring * 1.5f));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(-expand, -expand, expand, expand), 4, 4);
    }

    // Filled overlay tint
    QColor fill(255, 230, 60, (int)(pulse * 55));
    p.fillRect(r, fill);

    // "★" star icon in corner
    p.setPen(QColor(255, 230, 0, (int)(pulse * 230 + 25)));
    QFont f = p.font();
    f.setPixelSize((int)(r.height() * 0.35f));
    p.setFont(f);
    p.drawText(r.adjusted(2,2,-2,-2), Qt::AlignTop | Qt::AlignRight, "★");
}

// ── Player token ──────────────────────────────────────────────────────────────
void SimulationWindow::drawPlayerToken(QPainter& p, int idx, bool isActive, float phase) {
    if (!m_gameState) return;
    const PlayerInfo& player = m_gameState->players[idx];

    // Offset when multiple players share a cell
    static const QPointF offsets[4] = {
        {-12, -10}, {+12, -10}, {-12, +10}, {+12, +10}
    };
    QRectF cellR = boardSpaceRect(player.boardIndex);
    QPointF center = cellR.center() + offsets[idx % 4];

    float r = isActive ? kTokenRact : kTokenR;

    if (isActive) {
        // Glow rings
        float pulse = 0.5f + 0.5f * std::sin(phase * 3.0f);
        for (int ring = 4; ring >= 1; --ring) {
            float expand = ring * 5.0f * pulse;
            float alpha  = 0.4f / ring;
            QColor gc = player.color;
            gc.setAlphaF(alpha);
            p.setPen(Qt::NoPen);
            p.setBrush(gc);
            p.drawEllipse(center, r + expand, r + expand);
        }
    }

    // Token body
    QRadialGradient grad(center, r, center - QPointF(r * 0.35f, r * 0.35f));
    grad.setColorAt(0, player.color.lighter(160));
    grad.setColorAt(1, player.color);
    p.setBrush(grad);
    QPen outlinePen(isActive ? Qt::white : QColor(0,0,0,140), isActive ? 2.5f : 1.5f);
    p.setPen(outlinePen);
    p.drawEllipse(center, r, r);

    // Player initial inside token
    p.setPen(Qt::white);
    QFont f;
    f.setPixelSize((int)(r * 1.1f));
    f.setBold(true);
    p.setFont(f);
    QRectF tr(center.x()-r, center.y()-r, r*2, r*2);
    QString initial = player.name.isEmpty() ? "?" : player.name.left(1).toUpper();
    p.drawText(tr, Qt::AlignCenter, initial);
}

// ── Turn banner ───────────────────────────────────────────────────────────────
void SimulationWindow::drawTurnBanner(QPainter& p) {
    if (!m_gameState || m_gameState->players.isEmpty()) return;
    int pidx = m_gameState->currentPlayerIndex();
    if (pidx < 0) return;
    const PlayerInfo& player = m_gameState->players[pidx];

    QString text = QString("%1의 차례!!  주사위를 던지세요").arg(player.name);

    // Banner position: near top of screen
    QRect bannerRect(0, 12, width(), 60);

    // Semi-transparent background tinted with player color
    QColor bg = player.color;
    bg.setAlpha(200);
    p.fillRect(bannerRect, bg);

    // Pulsing border
    float pulse = 0.5f + 0.5f * std::sin(m_animPhase * 3.5f);
    p.setPen(QPen(QColor(255,255,255,(int)(pulse*220+35)), 2));
    p.drawRect(bannerRect);

    // Text
    QFont f;
    f.setPixelSize(28);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);

    // Shadow
    p.save();
    p.setPen(QColor(0,0,0,120));
    p.drawText(bannerRect.adjusted(2,2,2,2), Qt::AlignCenter, text);
    p.restore();
    p.setPen(Qt::white);
    p.drawText(bannerRect, Qt::AlignCenter, text);
}

// ── Intro overlay ─────────────────────────────────────────────────────────────
void SimulationWindow::drawIntroOverlay(QPainter& p) {
    if (!m_gameState) return;
    if (m_introPhase == IntroPhase::Playing) return;

    // Dim overlay
    p.fillRect(rect(), QColor(0, 0, 0, 160));

    QFont titleFont;
    titleFont.setPixelSize(54);
    titleFont.setBold(true);
    QFont subFont;
    subFont.setPixelSize(32);
    subFont.setBold(false);

    int cx = width() / 2;
    int cy = height() / 2;

    auto drawShadowText = [&](const QRect& r, const QString& text, const QFont& f,
                               QColor col, int align = Qt::AlignCenter) {
        p.setFont(f);
        p.setPen(QColor(0,0,0,150));
        p.drawText(r.adjusted(3,3,3,3), align, text);
        p.setPen(col);
        p.drawText(r, align, text);
    };

    switch (m_introPhase) {

    case IntroPhase::GameStart: {
        float pulse = 0.5f + 0.5f * std::sin(m_animPhase * 3.0f);
        QColor col(255, (int)(220 + pulse*35), 50);
        drawShadowText(QRect(0, cy-50, width(), 80), "🎮  게임 시작!", titleFont, col);
        break;
    }

    case IntroPhase::TurnAnnounce:
        drawShadowText(QRect(0, cy-50, width(), 80), "순서를 정합니다!", titleFont, Qt::white);
        break;

    case IntroPhase::TurnReveal: {
        drawShadowText(QRect(0, 40, width(), 60), "── 플레이 순서 ──", subFont, QColor(200,200,200));
        int cardH = 68, startY = cy - (m_gameState->turnOrder.size() * (cardH+10)) / 2;
        for (int i = 0; i < m_revealCount && i < m_gameState->turnOrder.size(); ++i) {
            int pidx = m_gameState->turnOrder[i];
            const PlayerInfo& player = m_gameState->players[pidx];
            QRect cardR(cx - 220, startY + i*(cardH+10), 440, cardH);
            // Card background
            QColor bg = player.color;
            bg.setAlpha(200);
            p.fillRect(cardR, bg);
            p.setPen(QPen(Qt::white, 2));
            p.drawRect(cardR);
            // Text
            p.setFont(subFont);
            p.setPen(Qt::white);
            p.drawText(cardR, Qt::AlignCenter,
                       QString("%1번째   %2").arg(i+1).arg(player.name));
        }
        break;
    }

    case IntroPhase::FestivalAnnounce: {
        float pulse = 0.5f + 0.5f * std::sin(m_animPhase * 4.0f);
        QColor col(50, (int)(200 + pulse*55), 100);
        drawShadowText(QRect(0, cy-50, width(), 80), "🎉  축제의 도시를 선정합니다!", titleFont, col);
        break;
    }

    case IntroPhase::FestivalReveal: {
        drawShadowText(QRect(0, 40, width(), 60), "── 축제의 도시 ──", subFont, QColor(255,230,80));
        int cardH = 68, startY = cy - (3*(cardH+10))/2;
        for (int i = 0; i < m_revealCount && i < m_gameState->festivalCities.size(); ++i) {
            int cidx = m_gameState->festivalCities[i];
            const QString& cityName = kBoardSpaces[cidx].name;
            QRect cardR(cx - 200, startY + i*(cardH+10), 400, cardH);
            p.fillRect(cardR, QColor(180, 130, 20, 200));
            p.setPen(QPen(QColor(255,230,80), 2));
            p.drawRect(cardR);
            QFont fn = subFont;
            fn.setPixelSize(36);
            drawShadowText(cardR, QString("★  %1").arg(cityName), fn, QColor(255,240,100));
        }
        break;
    }

    default:
        break;
    }
}

// ── Particles ─────────────────────────────────────────────────────────────────
void SimulationWindow::spawnFireworks(QPointF boardCenter, QColor color, int count) {
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < count; ++i) {
        Particle part;
        // Convert board center to screen coords for particle storage
        if (m_boardTransformValid)
            part.pos = m_boardTransform.map(boardCenter);
        else
            part.pos = boardCenter;

        float angle   = (float)rng->generateDouble() * 2.0f * (float)M_PI;
        float speed   = 30.0f + (float)rng->generateDouble() * 120.0f;
        part.vel      = QPointF(std::cos(angle) * speed, std::sin(angle) * speed);
        QColor c      = color;
        c.setHsvF(std::fmod(color.hsvHueF() + (rng->generateDouble()-0.5)*0.2, 1.0),
                  0.8f + rng->generateDouble()*0.2f,
                  0.9f + rng->generateDouble()*0.1f);
        part.color    = c;
        part.maxLife  = 0.8f + (float)rng->generateDouble() * 0.8f;
        part.life     = part.maxLife;
        m_particles.append(part);
    }
}

void SimulationWindow::drawParticles(QPainter& p) {
    for (const auto& part : m_particles) {
        float alpha = part.life / part.maxLife;
        QColor c    = part.color;
        c.setAlphaF(alpha);
        float r = 4.0f * alpha + 1.0f;
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(part.pos, r, r);
    }
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QPointF SimulationWindow::boardCellCenter(int cellIdx) const {
    return boardSpaceRect(cellIdx).center();
}

QPointF SimulationWindow::tokenCenter(int playerIdx) const {
    if (!m_gameState || playerIdx >= m_gameState->players.size()) return {};
    static const QPointF offsets[4] = {
        {-12, -10}, {+12, -10}, {-12, +10}, {+12, +10}
    };
    const PlayerInfo& pl = m_gameState->players[playerIdx];
    QPointF bCenter = boardSpaceRect(pl.boardIndex).center() + offsets[playerIdx % 4];
    if (m_boardTransformValid)
        return m_boardTransform.map(bCenter);
    return bCenter;
}

QPointF SimulationWindow::screenToBoard(QPointF screen) const {
    if (!m_boardTransformValid) return screen;
    bool ok;
    QTransform inv = m_boardTransform.inverted(&ok);
    return ok ? inv.map(screen) : screen;
}

int SimulationWindow::nearestBoardCell(QPointF boardPos) const {
    int best = 0;
    qreal bestDist = 1e9;
    for (int i = 0; i < kBoardTotal; ++i) {
        QPointF c = boardSpaceRect(i).center();
        qreal d = QLineF(boardPos, c).length();
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

// ── Mouse events ──────────────────────────────────────────────────────────────
void SimulationWindow::mousePressEvent(QMouseEvent* e) {
    if (!m_gameState || !m_gameState->isActive) return;
    if (m_introPhase != IntroPhase::Playing) return;
    if (e->button() != Qt::LeftButton) return;

    int pidx = m_gameState->currentPlayerIndex();
    if (pidx < 0) return;

    // Check if click is near active player's token
    QPointF click = e->position();
    QPointF center = tokenCenter(pidx);
    float radius = kTokenRact * 2.5f; // hit area in screen pixels (approximate)
    if (QLineF(click, center).length() < radius) {
        m_draggingPlayer = pidx;
        m_dragOffset     = center - click;
        m_gameState->players[pidx].dragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
}

void SimulationWindow::mouseMoveEvent(QMouseEvent* e) {
    if (m_draggingPlayer < 0 || !m_gameState) return;
    QPointF screenPos = e->position() + m_dragOffset;
    m_gameState->players[m_draggingPlayer].dragPos = screenPos;
    update();
}

void SimulationWindow::mouseReleaseEvent(QMouseEvent* e) {
    if (m_draggingPlayer < 0 || !m_gameState) return;
    if (e->button() != Qt::LeftButton) return;

    QPointF screenPos = e->position() + m_dragOffset;
    QPointF boardPos  = screenToBoard(screenPos);
    int newCell = nearestBoardCell(boardPos);
    m_gameState->players[m_draggingPlayer].boardIndex = newCell;
    m_gameState->players[m_draggingPlayer].dragging   = false;
    m_draggingPlayer = -1;
    unsetCursor();
    update();
}

// ── Keyboard ──────────────────────────────────────────────────────────────────
void SimulationWindow::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape)
        close();
    // Space: advance turn (convenience shortcut)
    if (e->key() == Qt::Key_Space && m_gameState &&
        m_introPhase == IntroPhase::Playing)
    {
        m_gameState->advanceTurn();
        SoundManager::instance().playPlayerTurn();
        update();
    }
}
