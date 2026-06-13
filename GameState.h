#pragma once
#include <QString>
#include <QColor>
#include <QVector>
#include <QPointF>

struct PlayerInfo {
    QString name;
    QColor  color;
    int     boardIndex{ 0 };
    QPointF dragPos;
    bool    dragging{ false };
};

struct GameState {
    QVector<PlayerInfo> players;
    QVector<int>        turnOrder;      // player indices in play order
    int                 turnStep{ 0 };  // index into turnOrder (mod players.size())
    QVector<int>        festivalCities; // 3 board cell indices

    bool isActive{ false };

    int currentPlayerIndex() const {
        if (turnOrder.isEmpty()) return -1;
        return turnOrder[turnStep % turnOrder.size()];
    }
    void advanceTurn() { ++turnStep; }
};
