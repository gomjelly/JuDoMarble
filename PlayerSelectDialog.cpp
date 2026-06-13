#include "PlayerSelectDialog.h"
#include "SoundManager.h"
#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PlayerSelectDialog::PlayerSelectDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("플레이어 선택");
    setModal(true);
    setFixedWidth(400);

    const QString colorNames[kMaxPlayers]   = { "파란색", "빨간색", "노란색", "초록색" };
    const QString defaultNames[kMaxPlayers] = { "파랑이", "빨강이", "노랑이", "초록이" };
    const QColor  colors[kMaxPlayers] = {
        QColor(60, 120, 220),
        QColor(220, 60, 60),
        QColor(220, 190, 30),
        QColor(50, 170, 70)
    };

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    mainLayout->addWidget(new QLabel("<b>참가할 플레이어를 선택하세요</b><br><small>(최소 2명)</small>"));

    auto* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(separator);

    for (int i = 0; i < kMaxPlayers; ++i) {
        m_rows[i].color = colors[i];

        auto* row = new QWidget;
        auto* hl  = new QHBoxLayout(row);
        hl->setContentsMargins(4, 4, 4, 4);
        hl->setSpacing(10);

        // Color swatch
        auto* swatch = new QLabel;
        swatch->setFixedSize(28, 28);
        swatch->setStyleSheet(QString("background:%1;border-radius:14px;border:2px solid #888;")
                                  .arg(colors[i].name()));

        // Checkbox
        auto* chk = new QCheckBox(colorNames[i]);
        chk->setChecked(i < 2); // default: first 2 checked
        m_rows[i].check = chk;

        // Name field
        auto* nameEdit = new QLineEdit(defaultNames[i]);
        nameEdit->setPlaceholderText("이름 입력");
        nameEdit->setEnabled(i < 2);
        m_rows[i].nameEdit = nameEdit;

        hl->addWidget(swatch);
        hl->addWidget(chk);
        hl->addWidget(nameEdit, 1);
        mainLayout->addWidget(row);

        int idx = i;
        connect(chk, &QCheckBox::toggled, this, [this, idx](bool v){ onToggle(idx, v); });
    }

    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(sep2);

    // Buttons
    auto* btnRow = new QWidget;
    auto* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);

    m_btnNext = new QPushButton("다음  ▶");
    m_btnNext->setStyleSheet("background:#2e7d32;color:white;padding:6px 18px;font-weight:bold;");
    auto* btnCancel = new QPushButton("취소");
    btnCancel->setStyleSheet("padding:6px 14px;");

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(m_btnNext);
    mainLayout->addWidget(btnRow);

    connect(m_btnNext,  &QPushButton::clicked, this, &PlayerSelectDialog::onNext);
    connect(btnCancel,  &QPushButton::clicked, this, &QDialog::reject);

    SoundManager::instance().playDialogOpen();
}

void PlayerSelectDialog::onToggle(int i, bool checked) {
    m_rows[i].nameEdit->setEnabled(checked);
    if (checked) m_rows[i].nameEdit->setFocus();
    validateNext();
}

void PlayerSelectDialog::validateNext() {
    int count = 0;
    for (int i = 0; i < kMaxPlayers; ++i)
        if (m_rows[i].check->isChecked()) ++count;
    m_btnNext->setEnabled(count >= 2);
}

void PlayerSelectDialog::onNext() {
    accept();
}

QVector<PlayerInfo> PlayerSelectDialog::selectedPlayers() const {
    QVector<PlayerInfo> result;
    for (int i = 0; i < kMaxPlayers; ++i) {
        if (!m_rows[i].check->isChecked()) continue;
        PlayerInfo p;
        p.color = m_rows[i].color;
        p.name  = m_rows[i].nameEdit->text().trimmed();
        if (p.name.isEmpty()) p.name = m_rows[i].check->text();
        p.boardIndex = 0;
        result.append(p);
    }
    return result;
}
