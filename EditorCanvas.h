#pragma once
#include <QWidget>
#include "PolygonItem.h"
#include "BuildingDialog.h"

class EditorCanvas : public QWidget {
    Q_OBJECT
public:
    explicit EditorCanvas(QWidget* parent = nullptr);

    void addRectangle();
    void addTriangle();
    void clearAll();
    void loadBoardState(bool hasBoard, const QPointF corners[4], const CellState states[kBoardTotal]);

    const QList<PolygonItem>& items()        const { return m_items; }
    bool                      hasBoard()      const { return m_showBoard; }
    const QPointF*            boardCorners()  const { return m_boardCorners; }
    const CellState*          cellStates()    const { return m_cellStates; }

    static constexpr int   CANVAS_W   = 1280;
    static constexpr int   CANVAS_H   = 720;
    static constexpr qreal BOARD_SIZE = 630.0;

    static void drawBoard(QPainter& p, const CellState* states = nullptr);
    static void drawBuildings(QPainter& p, const CellState* states);

signals:
    void boardChanged();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    QList<PolygonItem> m_items;
    int    m_selectedIndex{ -1 };
    int    m_dragVertexIndex{ -1 };
    int    m_dragBoardCorner{ -1 };
    bool   m_draggingPolygon{ false };
    bool   m_showBoard{ false };
    QPointF m_lastMousePos;
    QPointF m_boardCorners[4];   // TL, TR, BR, BL (canvas coords)
    CellState m_cellStates[kBoardTotal];

    static constexpr qreal HANDLE_R = 6.0;

    QTransform canvasToWidget() const;
    QTransform widgetToCanvas() const;
    QTransform boardTransform() const;

    int hitVertex(int itemIndex, QPointF widgetPos) const;
    int hitPolygon(QPointF canvasPos) const;
};
