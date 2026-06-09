#include "SimulationWindow.h"
#include "EditorCanvas.h"
#include "BoardData.h"
#include <QPainter>
#include <QKeyEvent>
#include <QScreen>
#include <QWindow>

SimulationWindow::SimulationWindow(EditorCanvas* canvas, QScreen* screen, QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
    , m_canvas(canvas)
{
    setWindowTitle("JuDoMarble");
    setGeometry(screen->geometry());
    show();
    windowHandle()->setScreen(screen);
    showFullScreen();

    // Repaint whenever the editor canvas changes
    connect(m_canvas, &EditorCanvas::boardChanged, this, QOverload<>::of(&QWidget::update));
}

void SimulationWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);

    QTransform canvasToScreen = QTransform::fromScale(
        (qreal)width()  / EditorCanvas::CANVAS_W,
        (qreal)height() / EditorCanvas::CANVAS_H
    );

    if (m_canvas->hasBoard()) {
        const qreal bs = kBoardSize;
        QPolygonF src, dst;
        src << QPointF(0,0) << QPointF(bs,0) << QPointF(bs,bs) << QPointF(0,bs);
        const QPointF* corners = m_canvas->boardCorners();
        for (int ci = 0; ci < 4; ++ci) dst << canvasToScreen.map(corners[ci]);
        QTransform boardT;
        if (QTransform::quadToQuad(src, dst, boardT)) {
            p.save();
            p.setTransform(boardT);
            EditorCanvas::drawBoard(p, m_canvas->cellStates(), m_canvas->allDownMode());
            EditorCanvas::drawBuildings(p, m_canvas->cellStates(), m_canvas->allDownMode());
            p.restore();
        }
    }

    QTransform ctw = canvasToScreen;
    for (const auto& item : m_canvas->items()) {
        QPolygonF poly = ctw.map(item.polygon);
        p.setBrush(item.color);
        p.setPen(QPen(Qt::white, 2));
        p.drawPolygon(poly);
    }
}

void SimulationWindow::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape)
        close();
}
