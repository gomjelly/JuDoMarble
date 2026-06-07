#pragma once
#include <QString>
#include "BuildingDialog.h"   // CellState, kBoardTotal

class EditorCanvas;

class GameSave {
public:
    // Returns true on success. Shows QMessageBox on error.
    static bool save(const QString& filePath, const EditorCanvas* canvas);
    static bool load(const QString& filePath, EditorCanvas* canvas);
};
