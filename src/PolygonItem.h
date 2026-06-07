#pragma once
#include <QPolygonF>
#include <QColor>
#include <QString>

struct PolygonItem {
    QPolygonF polygon;
    QColor color{ 100, 180, 255, 160 };
    QString label;
    bool selected{ false };
};
