#pragma once
#include <QMainWindow>

class EditorCanvas;
class QComboBox;
class QCheckBox;
class SimulationWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAddRectangle();
    void onAddTriangle();
    void onSimulate();
    void onStopSimulation();
    void onSave();
    void onLoad();

private:
    EditorCanvas* m_canvas{ nullptr };
    QComboBox* m_displayCombo{ nullptr };
    QCheckBox* m_allDownCheck{ nullptr };
    SimulationWindow* m_simWindow{ nullptr };

    void populateDisplayList();
};
