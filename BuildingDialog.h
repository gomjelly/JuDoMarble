#pragma once
#include <QDialog>
#include "BoardData.h"

// buildingCounts[i] = 각 건물 슬롯의 보유 수량
//   슬롯 0: 깃발       (최대 1)
//   슬롯 1: 빌라/파라솔 (복수 가능)
//   슬롯 2: 빌딩/멋진숙소(복수 가능)
//   슬롯 3: 호텔        (복수 가능)
//   슬롯 4: 랜드마크    (최대 1)
struct CellState {
    int buildingCounts[5]{ 0,0,0,0,0 };
    int playerColor{ -1 }; // -1=없음  0=빨강  1=파랑  2=초록  3=노랑

    bool hasAnyBuilding() const {
        for (int c : buildingCounts) if (c > 0) return true;
        return false;
    }
    int highestBuilding() const { // 가장 높은 등급 슬롯 반환 (-1=없음)
        for (int i = 4; i >= 0; --i) if (buildingCounts[i] > 0) return i;
        return -1;
    }
};

class QButtonGroup;
class QCheckBox;

class BuildingDialog : public QDialog {
    Q_OBJECT
public:
    explicit BuildingDialog(int spaceIdx, const CellState& current, QWidget* parent = nullptr);
    CellState result() const { return m_result; }

private:
    CellState     m_result;
    QCheckBox*    m_checks[5]{};
    QButtonGroup* m_colorGroup{ nullptr };
};
