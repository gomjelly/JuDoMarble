#include "GameSave.h"
#include "EditorCanvas.h"
#include "BoardData.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

static constexpr int kSaveVersion = 1;

bool GameSave::save(const QString& filePath, const EditorCanvas* canvas) {
    QJsonObject root;
    root["version"]  = kSaveVersion;
    root["hasBoard"] = canvas->hasBoard();

    // ── Board corners ─────────────────────────────────────────
    QJsonArray corners;
    const QPointF* bc = canvas->boardCorners();
    for (int i = 0; i < 4; ++i) {
        QJsonObject pt;
        pt["x"] = bc[i].x();
        pt["y"] = bc[i].y();
        corners.append(pt);
    }
    root["boardCorners"] = corners;

    // ── Cell states (only non-empty cells) ────────────────────
    QJsonObject cells;
    const CellState* states = canvas->cellStates();
    for (int i = 0; i < kBoardTotal; ++i) {
        if (!states[i].hasAnyBuilding() && states[i].playerColor < 0) continue;

        QJsonObject cell;
        QJsonArray counts;
        for (int c : states[i].buildingCounts) counts.append(c);
        cell["buildingCounts"] = counts;
        cell["playerColor"]    = states[i].playerColor;
        cells[QString::number(i)] = cell;
    }
    root["cellStates"] = cells;

    // ── Write file ────────────────────────────────────────────
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "저장 실패",
                             QString("파일을 열 수 없습니다:\n%1").arg(filePath));
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool GameSave::load(const QString& filePath, EditorCanvas* canvas) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "불러오기 실패",
                             QString("파일을 열 수 없습니다:\n%1").arg(filePath));
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(nullptr, "불러오기 실패",
                             QString("JSON 파싱 오류:\n%1").arg(err.errorString()));
        return false;
    }

    QJsonObject root = doc.object();
    if (root["version"].toInt() != kSaveVersion) {
        QMessageBox::warning(nullptr, "불러오기 실패", "지원하지 않는 세이브 파일 버전입니다.");
        return false;
    }

    // ── Board corners ─────────────────────────────────────────
    bool hasBoard = root["hasBoard"].toBool();
    QPointF corners[4];
    QJsonArray jarr = root["boardCorners"].toArray();
    if (jarr.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            QJsonObject pt = jarr[i].toObject();
            corners[i] = {pt["x"].toDouble(), pt["y"].toDouble()};
        }
    }

    // ── Cell states ───────────────────────────────────────────
    CellState states[kBoardTotal] = {};
    QJsonObject cells = root["cellStates"].toObject();
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        int idx = it.key().toInt();
        if (idx < 0 || idx >= kBoardTotal) continue;
        QJsonObject cell   = it.value().toObject();
        QJsonArray  counts = cell["buildingCounts"].toArray();
        for (int b = 0; b < 5 && b < counts.size(); ++b)
            states[idx].buildingCounts[b] = counts[b].toInt();
        states[idx].playerColor = cell["playerColor"].toInt(-1);
    }

    canvas->loadBoardState(hasBoard, corners, states);
    return true;
}
