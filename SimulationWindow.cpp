#include "SimulationWindow.h"
#include "EditorCanvas.h"
#include <QPainter>
#include <QKeyEvent>
#include <QScreen>
#include <QWindow>

SimulationWindow::SimulationWindow(const QList<PolygonItem>& items, QScreen* screen, QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint), m_items(items)
{
    setWindowTitle("JuDoMarble");
    // Move to target screen first, then go fullscreen so Qt picks the right screen
    setGeometry(screen->geometry());
    show();
    windowHandle()->setScreen(screen);
    showFullScreen();
}

void SimulationWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);

    QTransform t = QTransform::fromScale(
        (qreal)width() / EditorCanvas::CANVAS_W,
        (qreal)height() / EditorCanvas::CANVAS_H
    );

    for (const auto& item : m_items) {
        QPolygonF poly = t.map(item.polygon);
        p.setBrush(item.color);
        p.setPen(QPen(Qt::white, 2));
        p.drawPolygon(poly);
    }
}

void SimulationWindow::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape)
        close();
}
