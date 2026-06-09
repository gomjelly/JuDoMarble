#include "MainWindow.h"
#include "EditorCanvas.h"
#include "SimulationWindow.h"
#include "SoundManager.h"
#include "GameSave.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
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
    layout->addWidget(new QLabel("<b>세이브</b>"));
    auto* btnSave = new QPushButton("💾  저장");
    auto* btnLoad = new QPushButton("📂  불러오기");
    layout->addWidget(btnSave);
    layout->addWidget(btnLoad);

    layout->addSpacing(12);
    layout->addWidget(new QLabel("<b>텍스트 방향</b>"));
    m_allDownCheck = new QCheckBox("모두 아래방향");
    m_allDownCheck->setToolTip("체크: 모든 칸 텍스트를 같은 방향으로\n해제: 각 변 바깥쪽 방향으로");
    layout->addWidget(m_allDownCheck);

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
    connect(btnSave,  &QPushButton::clicked, this, &MainWindow::onSave);
    connect(btnLoad,  &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(m_allDownCheck, &QCheckBox::toggled, m_canvas, &EditorCanvas::setAllDownMode);

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
    SoundManager::instance().playSimulationStart();
    statusBar()->showMessage("시뮬레이션 실행 중  |  ESC로 종료");
}

void MainWindow::onSave() {
    QString path = QFileDialog::getSaveFileName(
        this, "게임 저장", "save.jdm",
        "JuDoMarble 세이브 (*.jdm);;JSON (*.json);;모든 파일 (*)");
    if (path.isEmpty()) return;
    if (GameSave::save(path, m_canvas))
        statusBar()->showMessage(QString("저장 완료: %1").arg(path));
}

void MainWindow::onLoad() {
    QString path = QFileDialog::getOpenFileName(
        this, "게임 불러오기", "",
        "JuDoMarble 세이브 (*.jdm);;JSON (*.json);;모든 파일 (*)");
    if (path.isEmpty()) return;
    if (GameSave::load(path, m_canvas))
        statusBar()->showMessage(QString("불러오기 완료: %1").arg(path));
}

void MainWindow::onStopSimulation() {
    if (m_simWindow) {
        delete m_simWindow;
        m_simWindow = nullptr;
        statusBar()->showMessage("시뮬레이션 종료");
    }
}
