#include "MainWindow.h"
#include "EditorCanvas.h"
#include "SimulationWindow.h"
#include <QComboBox>
#include <QDockWidget>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("JuDoMarble Editor");
    resize(1200, 750);

    m_canvas = new EditorCanvas(this);
    setCentralWidget(m_canvas);

    // Left tool panel
    auto* dock = new QDockWidget("도구", this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setFixedWidth(180);

    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(6);
    layout->setContentsMargins(8, 8, 8, 8);

    layout->addWidget(new QLabel("<b>보드</b>"));

    auto* btnRect  = new QPushButton("⊞  보드 생성");
    auto* btnClear = new QPushButton("전체 삭제");
    layout->addWidget(btnRect);
    layout->addWidget(btnClear);

    layout->addSpacing(12);
    layout->addWidget(new QLabel("<b>출력 디스플레이</b>"));
    m_displayCombo = new QComboBox;
    layout->addWidget(m_displayCombo);
    populateDisplayList();

    layout->addSpacing(12);
    auto* btnSim  = new QPushButton("▶  시뮬레이션 시작");
    auto* btnStop = new QPushButton("■  시뮬레이션 종료");
    btnSim->setStyleSheet("background:#2e7d32;color:white;padding:5px;");
    btnStop->setStyleSheet("background:#c62828;color:white;padding:5px;");
    layout->addWidget(btnSim);
    layout->addWidget(btnStop);

    layout->addStretch();
    layout->addWidget(new QLabel("<small>ESC: 시뮬레이션 종료</small>"));

    dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(btnRect,  &QPushButton::clicked, this, &MainWindow::onAddRectangle);
    connect(btnClear, &QPushButton::clicked, m_canvas, &EditorCanvas::clearAll);
    connect(btnSim,   &QPushButton::clicked, this, &MainWindow::onSimulate);
    connect(btnStop,  &QPushButton::clicked, this, &MainWindow::onStopSimulation);

    statusBar()->showMessage("준비  |  도형을 추가하고 꼭짓점을 드래그해 편집하세요");
}

void MainWindow::populateDisplayList() {
    m_displayCombo->clear();
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        auto* s = screens[i];
        m_displayCombo->addItem(
            QString("디스플레이 %1  (%2×%3)")
                .arg(i + 1).arg(s->geometry().width()).arg(s->geometry().height()),
            i
        );
    }
}

void MainWindow::onAddRectangle() {
    m_canvas->addRectangle();
    statusBar()->showMessage("보드 생성됨");
}

void MainWindow::onAddTriangle() {
    m_canvas->addTriangle();
    statusBar()->showMessage("삼각형 추가됨");
}

void MainWindow::onSimulate() {
    onStopSimulation();
    int idx = m_displayCombo->currentData().toInt();
    const auto screens = QGuiApplication::screens();
    if (idx < 0 || idx >= screens.size()) return;

    m_simWindow = new SimulationWindow(m_canvas, screens[idx]);
    m_simWindow->show();
    statusBar()->showMessage("시뮬레이션 실행 중  |  ESC로 종료");
}

void MainWindow::onStopSimulation() {
    if (m_simWindow) {
        delete m_simWindow;
        m_simWindow = nullptr;
        statusBar()->showMessage("시뮬레이션 종료");
    }
}
