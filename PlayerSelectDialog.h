#pragma once
#include <QDialog>
#include "GameState.h"

class QCheckBox;
class QLineEdit;
class QPushButton;

class PlayerSelectDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlayerSelectDialog(QWidget* parent = nullptr);
    QVector<PlayerInfo> selectedPlayers() const;

private:
    struct Row {
        QCheckBox* check{ nullptr };
        QLineEdit* nameEdit{ nullptr };
        QColor     color;
    };
    static constexpr int kMaxPlayers = 4;
    Row         m_rows[kMaxPlayers];
    QPushButton* m_btnNext{ nullptr };

    void onToggle(int i, bool checked);
    void onNext();
    void validateNext();
};
