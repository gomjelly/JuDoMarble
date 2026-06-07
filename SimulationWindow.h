#pragma once
#include <QWidget>
#include "PolygonItem.h"

class QScreen;

class SimulationWindow : public QWidget {
    Q_OBJECT
public:
    SimulationWindow(const QList<PolygonItem>& items, QScreen* screen, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    QList<PolygonItem> m_items;
};
