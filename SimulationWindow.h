#pragma once
#include <QWidget>
#include "PolygonItem.h"
#include "BuildingDialog.h"

class QScreen;
class EditorCanvas;

class SimulationWindow : public QWidget {
    Q_OBJECT
public:
    SimulationWindow(EditorCanvas* canvas, QScreen* screen, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    EditorCanvas* m_canvas;
};
