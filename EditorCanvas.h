#pragma once
#include <QWidget>
#include "PolygonItem.h"

class EditorCanvas : public QWidget {
    Q_OBJECT
public:
    explicit EditorCanvas(QWidget* parent = nullptr);

    void addRectangle();
    void addTriangle();
    void clearAll();

    const QList<PolygonItem>& items() const { return m_items; }

    static constexpr int CANVAS_W = 1280;
    static constexpr int CANVAS_H = 720;

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    QList<PolygonItem> m_items;
    int m_selectedIndex{ -1 };
    int m_dragVertexIndex{ -1 };
    bool m_draggingPolygon{ false };
    QPointF m_lastMousePos;

    static constexpr qreal HANDLE_R = 6.0;

    QTransform canvasToWidget() const;
    QTransform widgetToCanvas() const;

    int hitVertex(int itemIndex, QPointF widgetPos) const;
    int hitPolygon(QPointF canvasPos) const;
};
