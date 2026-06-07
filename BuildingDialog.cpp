#include "BuildingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QFrame>

static const struct { const char* label; const char* hex; } kPlayerColorDefs[4] = {
    {"빨강", "#DC143C"}, {"파랑", "#1E64C8"}, {"초록", "#32A032"}, {"노랑", "#F0C814"},
};

BuildingDialog::BuildingDialog(int spaceIdx, const CellState& current, QWidget* parent)
    : QDialog(parent), m_result(current)
{
    setWindowTitle("건물 설정");
    setFixedWidth(220);
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(5);
    lay->setContentsMargins(12, 12, 12, 12);

    // Header
    auto* title = new QLabel(QString("<b style='font-size:11pt'>%1</b>")
                             .arg(kBoardSpaces[spaceIdx].name));
    title->setAlignment(Qt::AlignCenter);
    lay->addWidget(title);

    auto* line = new QFrame; line->setFrameShape(QFrame::HLine);
    lay->addWidget(line);

    // ── Buildings (all checkboxes) ────────────────────────────────
    const QStringList buildings = buildingNames(spaceIdx);
    if (buildings.isEmpty()) {
        lay->addWidget(new QLabel("<i>이 칸에는 건물을 지을 수 없습니다.</i>"));
    } else {
        lay->addWidget(new QLabel("<b>건물</b>"));
        for (int i = 0; i < buildings.size(); ++i) {
            auto* cb = new QCheckBox(buildings[i]);
            cb->setChecked(current.buildingCounts[i] > 0);
            m_checks[i] = cb;
            lay->addWidget(cb);
        }
    }

    lay->addSpacing(4);

    // ── Player color ──────────────────────────────────────────────
    m_colorGroup = new QButtonGroup(this);
    lay->addWidget(new QLabel("<b>플레이어</b>"));

    auto* noColor = new QRadioButton("없음");
    m_colorGroup->addButton(noColor, -1);
    noColor->setChecked(current.playerColor == -1);
    lay->addWidget(noColor);

    auto* colorRow = new QHBoxLayout;
    colorRow->setSpacing(4);
    for (int i = 0; i < 4; ++i) {
        auto* btn = new QPushButton(kPlayerColorDefs[i].label);
        btn->setCheckable(true);
        btn->setFixedSize(42, 28);
        QColor c(kPlayerColorDefs[i].hex);
        btn->setStyleSheet(
            QString("QPushButton{background:%1;color:%2;border:2px solid transparent;"
                    "border-radius:4px;font-weight:bold;font-size:8pt;}"
                    "QPushButton:checked{border:2px solid black;}")
            .arg(kPlayerColorDefs[i].hex)
            .arg(c.lightness() < 128 ? "white" : "black"));
        m_colorGroup->addButton(btn, i);
        btn->setChecked(current.playerColor == i);
        colorRow->addWidget(btn);
    }
    lay->addLayout(colorRow);

    lay->addSpacing(6);

    // ── OK / Cancel ───────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, this, [this] {
        for (int i = 0; i < 5; ++i)
            m_result.buildingCounts[i] = (m_checks[i] && m_checks[i]->isChecked()) ? 1 : 0;
        m_result.playerColor = m_colorGroup->checkedId();
        accept();
    });
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(btns);
}
