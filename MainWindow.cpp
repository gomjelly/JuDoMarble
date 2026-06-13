#include "MainWindow.h"
#include "EditorCanvas.h"
#include "SimulationWindow.h"
#include "PlayerSelectDialog.h"
#include "SoundManager.h"
#include "GameSave.h"
#include "BoardData.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScreen>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("JuDoMarble Editor");
    resize(1200, 750);

    m_canvas = new EditorCanvas(this);
    setCentralWidget(m_canvas);

    auto* dock = new QDockWidget("도구", this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    dock->setFixedWidth(180);

    auto* panel  = new QWidget;
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
    layout->addWidget(new QLabel("<b>게임</b>"));
    auto* btnGame = new QPushButton("🎮  게임 시작");
    btnGame->setStyleSheet("background:#1565c0;color:white;padding:5px;font-weight:bold;");
    layout->addWidget(btnGame);

    layout->addSpacing(6);
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
    layout->addWidget(new QLabel("<small>ESC: 종료  |  Space: 차례 넘기기</small>"));

    dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(btnRect,  &QPushButton::clicked, this, &MainWindow::onAddRectangle);
    connect(btnClear, &QPushButton::clicked, m_canvas, &EditorCanvas::clearAll);
    connect(btnSim,   &QPushButton::clicked, this, &MainWindow::onSimulate);
    connect(btnStop,  &QPushButton::clicked, this, &MainWindow::onStopSimulation);
    connect(btnSave,  &QPushButton::clicked, this, &MainWindow::onSave);
    connect(btnLoad,  &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(btnGame,  &QPushButton::clicked, this, &MainWindow::onStartGame);
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
            i);
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

void MainWindow::onStartGame() {
    if (!m_canvas->hasBoard()) {
        QMessageBox::warning(this, "게임 시작", "먼저 보드를 생성해주세요.");
        return;
    }

    PlayerSelectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QVector<PlayerInfo> players = dlg.selectedPlayers();
    if (players.size() < 2) return;

    setupGameState(players);

    // Start simulation with game state
    onStopSimulation();
    int idx = m_displayCombo->currentData().toInt();
    const auto screens = QGuiApplication::screens();
    if (idx < 0 || idx >= screens.size()) return;

    m_simWindow = new SimulationWindow(m_canvas, screens[idx], &m_gameState);
    m_simWindow->show();
    statusBar()->showMessage("게임 시작!  |  ESC로 종료  |  Space로 차례 넘기기");
}

void MainWindow::setupGameState(QVector<PlayerInfo> players) {
    m_gameState = GameState{};
    m_gameState.players  = players;
    m_gameState.isActive = true;

    // Random turn order
    QVector<int> order;
    for (int i = 0; i < players.size(); ++i) order.append(i);
    auto* rng = QRandomGenerator::global();
    for (int i = order.size() - 1; i > 0; --i) {
        int j = rng->bounded(i + 1);
        std::swap(order[i], order[j]);
    }
    m_gameState.turnOrder = order;

    // Random 3 festival cities (cities where buildings can be built, non-island)
    QVector<int> candidates;
    for (int i = 0; i < kBoardTotal; ++i) {
        if (canBuildHere(i) && !isIslandCell(i))
            candidates.append(i);
    }
    // Shuffle candidates and pick first 3
    for (int i = candidates.size() - 1; i > 0; --i) {
        int j = rng->bounded(i + 1);
        std::swap(candidates[i], candidates[j]);
    }
    m_gameState.festivalCities.clear();
    for (int i = 0; i < 3 && i < candidates.size(); ++i)
        m_gameState.festivalCities.append(candidates[i]);
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
