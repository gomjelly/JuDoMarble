#include "EditorCanvas.h"
#include <QPainter>
#include <QMouseEvent>

EditorCanvas::EditorCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 360);
    setMouseTracking(true);
    setStyleSheet("background-color: #1a1a2e;");
}

void EditorCanvas::addRectangle() {
    qreal cx = CANVAS_W / 2.0, cy = CANVAS_H / 2.0;
    PolygonItem item;
    item.polygon << QPointF(cx - 120, cy - 80) << QPointF(cx + 120, cy - 80)
                 << QPointF(cx + 120, cy + 80) << QPointF(cx - 120, cy + 80);
    m_items.append(item);
    update();
}

void EditorCanvas::addTriangle() {
    qreal cx = CANVAS_W / 2.0, cy = CANVAS_H / 2.0;
    PolygonItem item;
    item.polygon << QPointF(cx, cy - 100) << QPointF(cx + 110, cy + 80)
                 << QPointF(cx - 110, cy + 80);
    item.color = QColor(255, 160, 60, 160);
    m_items.append(item);
    update();
}

void EditorCanvas::clearAll() {
    m_items.clear();
    m_selectedIndex = -1;
    m_dragVertexIndex = -1;
    m_draggingPolygon = false;
    update();
}

QTransform EditorCanvas::canvasToWidget() const {
    return QTransform::fromScale((qreal)width() / CANVAS_W, (qreal)height() / CANVAS_H);
}

QTransform EditorCanvas::widgetToCanvas() const {
    return canvasToWidget().inverted();
}

int EditorCanvas::hitVertex(int idx, QPointF widgetPos) const {
    if (idx < 0 || idx >= m_items.size()) return -1;
    auto ctw = canvasToWidget();
    const auto& poly = m_items[idx].polygon;
    for (int i = 0; i < poly.size(); ++i) {
        if (QLineF(ctw.map(poly[i]), widgetPos).length() <= HANDLE_R + 3)
            return i;
    }
    return -1;
}

int EditorCanvas::hitPolygon(QPointF canvasPos) const {
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].polygon.containsPoint(canvasPos, Qt::OddEvenFill))
            return i;
    }
    return -1;
}

void EditorCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background + grid
    p.fillRect(rect(), QColor(26, 26, 46));
    p.setPen(QColor(45, 45, 75));
    for (int x = 0; x < width(); x += 40) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 40) p.drawLine(0, y, width(), y);

    // Canvas boundary
    auto ctw = canvasToWidget();
    p.setPen(QPen(QColor(80, 80, 120), 2, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(ctw.map(QPointF(0, 0)), ctw.map(QPointF(CANVAS_W, CANVAS_H))));

    for (int i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        QPolygonF wp = ctw.map(item.polygon);

        p.setBrush(item.color);
        p.setPen(QPen(item.selected ? QColor(255, 220, 0) : Qt::white,
                      item.selected ? 2.0 : 1.0));
        p.drawPolygon(wp);

        if (item.selected) {
            p.setBrush(QColor(255, 220, 0));
            p.setPen(QPen(Qt::black, 1));
            for (const QPointF& v : wp)
                p.drawEllipse(v, HANDLE_R, HANDLE_R);
        }
    }
}

void EditorCanvas::mousePressEvent(QMouseEvent* e) {
    QPointF wp = e->position();
    QPointF cp = widgetToCanvas().map(wp);

    // Vertex drag check on selected item
    if (m_selectedIndex >= 0) {
        int vi = hitVertex(m_selectedIndex, wp);
        if (vi >= 0) {
            m_dragVertexIndex = vi;
            m_lastMousePos = wp;
            return;
        }
    }

    int hit = hitPolygon(cp);
    if (hit >= 0) {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
            m_items[m_selectedIndex].selected = false;
        m_selectedIndex = hit;
        m_items[hit].selected = true;
        m_draggingPolygon = true;
        m_lastMousePos = wp;
    } else {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
            m_items[m_selectedIndex].selected = false;
        m_selectedIndex = -1;
    }
    update();
}

void EditorCanvas::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragVertexIndex >= 0 && m_selectedIndex >= 0) {
        m_items[m_selectedIndex].polygon[m_dragVertexIndex] =
            widgetToCanvas().map(e->position());
        update();
    } else if (m_draggingPolygon && m_selectedIndex >= 0) {
        QPointF delta = e->position() - m_lastMousePos;
        // convert delta from widget space to canvas space (pure scale, no translation)
        QPointF cd = widgetToCanvas().map(delta) - widgetToCanvas().map(QPointF(0, 0));
        for (auto& pt : m_items[m_selectedIndex].polygon)
            pt += cd;
        m_lastMousePos = e->position();
        update();
    }
}

void EditorCanvas::mouseReleaseEvent(QMouseEvent*) {
    m_dragVertexIndex = -1;
    m_draggingPolygon = false;
}
