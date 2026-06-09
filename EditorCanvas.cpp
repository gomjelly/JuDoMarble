#include "EditorCanvas.h"
#include "BoardData.h"
#include "SoundManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <cmath>

EditorCanvas::EditorCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(640, 360);
    setMouseTracking(true);
    setStyleSheet("background-color: #1a1a2e;");
}

void EditorCanvas::addRectangle() {
    m_showBoard = true;
    const qreal bs = BOARD_SIZE;
    const qreal bx = (CANVAS_W - bs) / 2.0;
    const qreal by = (CANVAS_H - bs) / 2.0;
    m_boardCorners[0] = {bx,    by};
    m_boardCorners[1] = {bx+bs, by};
    m_boardCorners[2] = {bx+bs, by+bs};
    m_boardCorners[3] = {bx,    by+bs};
    update();
    emit boardChanged();
}

void EditorCanvas::addTriangle() {
    qreal cx = CANVAS_W / 2.0, cy = CANVAS_H / 2.0;
    PolygonItem item;
    item.polygon << QPointF(cx, cy - 100) << QPointF(cx + 110, cy + 80)
                 << QPointF(cx - 110, cy + 80);
    item.color = QColor(255, 160, 60, 160);
    m_items.append(item);
    update();
}

void EditorCanvas::loadBoardState(bool hasBoard, const QPointF corners[4], const CellState states[kBoardTotal]) {
    m_showBoard = hasBoard;
    for (int i = 0; i < 4; ++i)        m_boardCorners[i] = corners[i];
    for (int i = 0; i < kBoardTotal; ++i) m_cellStates[i] = states[i];
    m_selectedIndex   = -1;
    m_dragVertexIndex = -1;
    m_dragBoardCorner = -1;
    m_draggingPolygon = false;
    update();
    emit boardChanged();
}

void EditorCanvas::clearAll() {
    m_items.clear();
    m_showBoard = false;
    m_selectedIndex = -1;
    m_dragVertexIndex = -1;
    m_dragBoardCorner = -1;
    m_draggingPolygon = false;
    update();
    emit boardChanged();
}

QTransform EditorCanvas::canvasToWidget() const {
    qreal scale = qMin((qreal)width() / CANVAS_W, (qreal)height() / CANVAS_H);
    qreal ox = (width()  - CANVAS_W * scale) / 2.0;
    qreal oy = (height() - CANVAS_H * scale) / 2.0;
    return QTransform::fromScale(scale, scale) * QTransform::fromTranslate(ox, oy);
}

QTransform EditorCanvas::widgetToCanvas() const {
    return canvasToWidget().inverted();
}

QTransform EditorCanvas::boardTransform() const {
    const qreal bs = kBoardSize;
    QPolygonF src, dst;
    src << QPointF(0,0) << QPointF(bs,0) << QPointF(bs,bs) << QPointF(0,bs);
    auto ctw = canvasToWidget();
    for (const auto& c : m_boardCorners) dst << ctw.map(c);
    QTransform t;
    QTransform::quadToQuad(src, dst, t);
    return t;
}

int EditorCanvas::hitVertex(int idx, QPointF widgetPos) const {
    if (idx < 0 || idx >= m_items.size()) return -1;
    auto ctw = canvasToWidget();
    const auto& poly = m_items[idx].polygon;
    for (int i = 0; i < poly.size(); ++i) {
        if (QLineF(ctw.map(poly[i]), widgetPos).length() <= HANDLE_R + 3)
            return i;
    }
    return -1;
}

int EditorCanvas::hitPolygon(QPointF canvasPos) const {
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].polygon.containsPoint(canvasPos, Qt::OddEvenFill))
            return i;
    }
    return -1;
}

void EditorCanvas::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background + grid
    p.fillRect(rect(), QColor(26, 26, 46));
    p.setPen(QColor(45, 45, 75));
    for (int x = 0; x < width(); x += 40) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 40) p.drawLine(0, y, width(), y);

    // Canvas boundary
    auto ctw = canvasToWidget();
    p.setPen(QPen(QColor(80, 80, 120), 2, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(ctw.map(QPointF(0, 0)), ctw.map(QPointF(CANVAS_W, CANVAS_H))));

    if (m_showBoard) {
        const qreal bs = BOARD_SIZE;
        QPolygonF src, dst;
        src << QPointF(0,0) << QPointF(bs,0) << QPointF(bs,bs) << QPointF(0,bs);
        auto ctw = canvasToWidget();
        for (const auto& c : m_boardCorners) dst << ctw.map(c);
        QTransform boardT;
        if (QTransform::quadToQuad(src, dst, boardT)) {
            p.save();
            p.setTransform(boardT);
            drawBoard(p, m_cellStates, m_allDownMode);
            drawBuildings(p, m_cellStates, m_allDownMode);
            p.restore();
        }
        // Corner handles
        p.setPen(QPen(QColor(255,80,80), 2));
        p.setBrush(QColor(255,80,80, 200));
        for (const auto& c : m_boardCorners)
            p.drawEllipse(ctw.map(c), HANDLE_R, HANDLE_R);
    }

    for (int i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        QPolygonF wp = ctw.map(item.polygon);

        p.setBrush(item.color);
        p.setPen(QPen(item.selected ? QColor(255, 220, 0) : Qt::white,
                      item.selected ? 2.0 : 1.0));
        p.drawPolygon(wp);

        if (item.selected) {
            p.setBrush(QColor(255, 220, 0));
            p.setPen(QPen(Qt::black, 1));
            for (const QPointF& v : wp)
                p.drawEllipse(v, HANDLE_R, HANDLE_R);
        }
    }
}

void EditorCanvas::mousePressEvent(QMouseEvent* e) {
    QPointF wp = e->position();
    QPointF cp = widgetToCanvas().map(wp);

    // Board corner drag check
    if (m_showBoard) {
        auto ctw = canvasToWidget();
        for (int i = 0; i < 4; ++i) {
            if (QLineF(ctw.map(m_boardCorners[i]), wp).length() <= HANDLE_R + 4) {
                m_dragBoardCorner = i;
                return;
            }
        }
    }

    // Board cell click → open building dialog
    if (m_showBoard && e->button() == Qt::LeftButton) {
        bool ok = false;
        QTransform inv = boardTransform().inverted(&ok);
        if (ok) {
            QPointF canonical = inv.map(wp);
            for (int i = 0; i < kBoardTotal; ++i) {
                if (boardSpaceRect(i).contains(canonical)) {
                    if (canBuildHere(i)) {
                        SoundManager::instance().playDialogOpen();
                        BuildingDialog dlg(i, m_cellStates[i], this);
                        {
                            QPoint gp = mapToGlobal(wp.toPoint()) + QPoint(12, -180);
                            QSize  ds = dlg.sizeHint();
                            QRect  screen = QGuiApplication::primaryScreen()->availableGeometry();
                            gp.setX(qMin(gp.x(), screen.right()  - ds.width()));
                            gp.setY(qMin(gp.y(), screen.bottom() - ds.height()));
                            gp.setX(qMax(gp.x(), screen.left()));
                            gp.setY(qMax(gp.y(), screen.top()));
                            dlg.move(gp);
                        }
                        if (dlg.exec() == QDialog::Accepted) {
                            m_cellStates[i] = dlg.result();
                            if (m_cellStates[i].hasAnyBuilding())
                                SoundManager::instance().playBuildingPlaced();
                        }
                        update();
                        emit boardChanged();
                    }
                    return;
                }
            }
        }
    }

    // Vertex drag check on selected item
    if (m_selectedIndex >= 0) {
        int vi = hitVertex(m_selectedIndex, wp);
        if (vi >= 0) {
            m_dragVertexIndex = vi;
            m_lastMousePos = wp;
            return;
        }
    }

    int hit = hitPolygon(cp);
    if (hit >= 0) {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
            m_items[m_selectedIndex].selected = false;
        m_selectedIndex = hit;
        m_items[hit].selected = true;
        m_draggingPolygon = true;
        m_lastMousePos = wp;
    } else {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
            m_items[m_selectedIndex].selected = false;
        m_selectedIndex = -1;
    }
    update();
}

void EditorCanvas::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragBoardCorner >= 0) {
        m_boardCorners[m_dragBoardCorner] = widgetToCanvas().map(e->position());
        update();
        emit boardChanged();
        return;
    }
    if (m_dragVertexIndex >= 0 && m_selectedIndex >= 0) {
        m_items[m_selectedIndex].polygon[m_dragVertexIndex] =
            widgetToCanvas().map(e->position());
        update();
    } else if (m_draggingPolygon && m_selectedIndex >= 0) {
        QPointF delta = e->position() - m_lastMousePos;
        // convert delta from widget space to canvas space (pure scale, no translation)
        QPointF cd = widgetToCanvas().map(delta) - widgetToCanvas().map(QPointF(0, 0));
        for (auto& pt : m_items[m_selectedIndex].polygon)
            pt += cd;
        m_lastMousePos = e->position();
        update();
    }
}

void EditorCanvas::mouseReleaseEvent(QMouseEvent*) {
    m_dragBoardCorner = -1;
    m_dragVertexIndex = -1;
    m_draggingPolygon = false;
}

// ── Board drawing ─────────────────────────────────────────────────────────────
// drawBoard() renders in canonical (0,0)–(kBoardSize,kBoardSize) space.

namespace {

// ── Corner cell drawing (clean 2-line style) ──────────────────────────────────

static void drawCornerCell(QPainter& p, const QRectF& r, int idx, bool allDown) {
    const auto& sp = kBoardSpaces[idx];
    p.fillRect(r, sp.color);
    p.setPen(QPen(QColor(40,40,40), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    static const QString kEngLabels[4] = {"START", "ISLAND", "OLYMPIC", "WORLD"};
    // 각 코너의 바깥 대각선 방향 (BR=-45, BL=+45, TL=+135, TR=-135)
    static const int kDiagRot[4] = {-45, 45, 135, -135};

    int sideIdx = idx / 8;
    QColor fg = sp.color.lightness() < 155 ? Qt::white : QColor(40,40,40);
    int rotDeg = allDown ? 0 : kDiagRot[sideIdx];

    p.save();
    p.setClipRect(r);
    p.translate(r.center());
    p.rotate(rotDeg);
    qreal half = r.width() / 2.0; // 코너는 정사각형(90×90)

    QFont nf("맑은 고딕"); nf.setPixelSize(18); nf.setBold(true);
    nf.setStyleStrategy(QFont::PreferAntialias);
    p.setFont(nf); p.setPen(fg);
    p.drawText(QRectF(-half, -half*0.55, half*2, half*0.65), Qt::AlignCenter, sp.name);

    QFont ef("맑은 고딕"); ef.setPixelSize(11); ef.setBold(false);
    ef.setStyleStrategy(QFont::PreferAntialias);
    p.setFont(ef); p.setPen(fg);
    p.drawText(QRectF(-half, half*0.08, half*2, half*0.45), Qt::AlignCenter, kEngLabels[sideIdx]);

    p.restore();
}

// ── Non-corner cell drawing (clean text-centric style) ────────────────────────

static void drawFortuneCard(QPainter& p, const QRectF& local) {
    int minDim = (int)qMin(local.width(), local.height());
    QFont sf("맑은 고딕"); sf.setPixelSize(qMax(12, (int)(minDim*0.26)));
    sf.setBold(true); sf.setStyleStrategy(QFont::PreferAntialias);
    p.setFont(sf); p.setPen(QColor(180,110,0));
    p.drawText(QRectF(local.left(), local.top(), local.width(), local.height()*0.56),
               Qt::AlignCenter, "★");
    QFont tf("맑은 고딕"); tf.setPixelSize(qMax(8, (int)(minDim*0.16)));
    tf.setBold(true); tf.setStyleStrategy(QFont::PreferAntialias);
    p.setFont(tf); p.setPen(QColor(110,65,0));
    p.drawText(QRectF(local.left(), local.top()+local.height()*0.52, local.width(), local.height()*0.48),
               Qt::AlignCenter, "포춘카드");
}

static void paintCell(QPainter& p, const QRectF& r, int idx, int rotDeg, bool hasBuilding = false) {
    const auto& sp = kBoardSpaces[idx];
    p.fillRect(r, sp.color);
    p.setPen(QPen(QColor(50,50,50), 0.8));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    if (hasBuilding) return; // 건물 있으면 텍스트 생략

    p.save();
    p.translate(r.center());
    p.rotate(rotDeg);
    bool is90 = (rotDeg == 90 || rotDeg == -90);
    QRectF local(-(is90 ? r.height() : r.width())/2,
                 -(is90 ? r.width()  : r.height())/2,
                   is90 ? r.height() : r.width(),
                   is90 ? r.width()  : r.height());

    if (sp.type == SpaceType::FortuneCard) {
        drawFortuneCard(p, local);
    } else {
        QColor fg = sp.color.lightness() < 155 ? Qt::white : QColor(30,30,30);
        int fs = qMin(14, qMax(9, (int)(qMin(local.width(), local.height()) * 0.22)));
        QFont f("맑은 고딕"); f.setPixelSize(fs); f.setBold(true);
        f.setStyleStrategy(QFont::PreferAntialias);
        p.setFont(f); p.setPen(fg);
        // 보너스게임은 두 줄로 표시
        QString displayName = sp.name;
        if (displayName == "보너스게임") displayName = "보너스\n게임";
        p.drawText(local.adjusted(2,2,-2,-2), Qt::AlignCenter | Qt::TextWordWrap, displayName);
    }
    p.restore();
}

} // namespace

void EditorCanvas::drawBoard(QPainter& p, const CellState* states, bool allDown) {
    p.setRenderHint(QPainter::Antialiasing);

    const qreal bs = kBoardSize, cs = kCornerSize;

    // Board background (clean off-white)
    p.fillRect(QRectF(0, 0, bs, bs), QColor(252,250,245));

    // Center circle
    QRectF center(cs, cs, bs-2*cs, bs-2*cs);
    QPointF ctr = center.center();
    qreal radius = qMin(center.width(), center.height()) * 0.47;
    p.setBrush(QColor(100,160,225));
    p.setPen(Qt::NoPen);
    p.drawEllipse(ctr, radius, radius);
    {
        QFont f("맑은 고딕"); f.setPixelSize(62); f.setBold(true);
        f.setStyleStrategy(QFont::PreferAntialias);
        p.setFont(f); p.setPen(Qt::white);
        p.drawText(QRectF(ctr.x()-radius, ctr.y()-radius*0.9, radius*2, radius*1.1),
                   Qt::AlignCenter, "MARBLE");
        QFont sf("맑은 고딕"); sf.setPixelSize(20); sf.setBold(false);
        sf.setStyleStrategy(QFont::PreferAntialias);
        p.setFont(sf);
        p.drawText(QRectF(ctr.x()-radius, ctr.y()+radius*0.1, radius*2, radius*0.5),
                   Qt::AlignCenter, "주사위를 굴려라!");
    }

    // Rotation per side (outward mode): side0=bottom(0), side1=left(90), side2=top(180), side3=right(-90)
    const int kRot[4] = {0, 90, 180, -90};

    for (int i = 0; i < kBoardTotal; ++i) {
        QRectF r = boardSpaceRect(i);
        if (i % 8 == 0) {
            drawCornerCell(p, r, i, allDown);
        } else {
            int rot = allDown ? 0 : kRot[i / 8];
            bool hasBuilding = states && states[i].hasAnyBuilding() && states[i].playerColor >= 0;
            paintCell(p, r, i, rot, hasBuilding);
        }
    }

    // Outer border
    p.setPen(QPen(QColor(30,30,30), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(0, 0, bs, bs));
}

// ── Building rendering ────────────────────────────────────────────────────────

namespace {

static const QColor kPlayerColors[4] = {
    {220, 20, 60}, {30,100,200}, {50,160, 50}, {230,190,  0},
};

// ── Sprite sheet ──────────────────────────────────────────────────────────────
// Grid: 6 cols × 4 rows, each cell 256×256
// Row 0: 깃발(0) 파라솔(1) 멋진숙소(2) 빌라(3) 빌딩(4) 호텔(5)
// Row 1: 방콕(0) 베이징(1) 타이페이(2) 두바이(3) 카이로(4) 도쿄(5)
// Row 2: 시드니(0) 퀘벡(1) 상파울루(2) 프라하(3) 베를린(4) 모스크바(5)
// Row 3: 제네바(0) 로마(1) 런던(2) 파리(3) 뉴욕(4) 서울(5)

// playerColor 0=빨강 1=파랑 2=초록 3=노랑
// Alpha 채널 보존을 위해 QImage::Format_ARGB32로 강제 변환 후 로드
// 밝은 회색/흰색(체커보드 배경) 픽셀을 투명으로 제거
static QPixmap loadSheet(const char* path) {
    QImage img(path);
    if (img.isNull()) return {};
    QImage result = img.convertToFormat(QImage::Format_ARGB32);

    for (int y = 0; y < result.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            // 흰색 배경 + 안티앨리어싱 노이즈 → 투명 처리
            if (r >= 230 && g >= 230 && b >= 230)
                line[x] = qRgba(0, 0, 0, 0);
        }
    }
    return QPixmap::fromImage(result);
}

static const QPixmap& spriteSheet(int playerColor) {
    static QPixmap px[4] = {
        loadSheet(":/resource/Generated_Image_red.png"),
        loadSheet(":/resource/Generated_Image_blue.png"),
        loadSheet(":/resource/Generated_Image_green.png"),
        loadSheet(":/resource/Generated_Image_yellow.png"),
    };
    return (playerColor >= 0 && playerColor < 4) ? px[playerColor] : px[0];
}

// 셀 크기를 이미지에서 동적으로 계산 (6×4 그리드)
static QRect spriteCell(const QPixmap& sheet, int col, int row) {
    int cw = sheet.width()  / 6;
    int ch = sheet.height() / 4;
    return QRect(col * cw, row * ch, cw, ch);
}

// 투명 배경 유지하며 스프라이트 셀 그리기
static void drawSprite(QPainter& p, int col, int row, const QRectF& dst, int playerColor) {
    const QPixmap& sheet = spriteSheet(playerColor);
    if (sheet.isNull()) return;
    QRect src = spriteCell(sheet, col, row);
    p.drawPixmap(dst, sheet, QRectF(src));
}

// Landmark sprite mapping: kLandmarkSprite[spaceIdx] = {col, row}, {-1,-1} = no sprite
static const int kLandmarkSprite[32][2] = {
    {-1,-1}, // 0  출발
    { 0, 1}, // 1  방콕
    {-1,-1}, // 2  보너스
    { 1, 1}, // 3  베이징
    {-1,-1}, // 4  독도 (섬)
    { 2, 1}, // 5  타이페이
    { 3, 1}, // 6  두바이
    { 4, 1}, // 7  카이로
    {-1,-1}, // 8  무인도
    {-1,-1}, // 9  발리 (섬)
    { 5, 1}, // 10 도쿄
    { 0, 2}, // 11 시드니
    {-1,-1}, // 12 포춘카드
    { 1, 2}, // 13 퀘벡
    {-1,-1}, // 14 하와이 (섬)
    { 2, 2}, // 15 상파울루
    {-1,-1}, // 16 올림픽
    { 3, 2}, // 17 프라하
    {-1,-1}, // 18 푸켓 (섬)
    { 4, 2}, // 19 베를린
    {-1,-1}, // 20 포춘카드
    { 5, 2}, // 21 모스크바
    { 0, 3}, // 22 제네바
    { 1, 3}, // 23 로마
    {-1,-1}, // 24 세계여행
    {-1,-1}, // 25 타히티 (섬)
    { 2, 3}, // 26 런던
    { 3, 3}, // 27 파리
    {-1,-1}, // 28 포춘카드
    { 4, 3}, // 29 뉴욕
    {-1,-1}, // 30 국세청
    { 5, 3}, // 31 서울
};

// ── Shared: icon dst rect centered slightly above cell center ─────────────────
static QRectF iconDst(const QRectF& local, qreal fillRatio = 0.82) {
    qreal sz = qMin(local.width(), local.height()) * fillRatio;
    return QRectF(-sz * 0.5, -local.height() * 0.44, sz, sz);
}

// ── Landmark drawings (slot 4) — sprite first, QPainter fallback ──────────────
static void drawLandmarkIcon(QPainter& p, qreal cx, qreal cy, qreal s, int spaceIdx, QColor col, int playerColor = -1) {
    // Try sprite first
    if (spaceIdx >= 0 && spaceIdx < 32) {
        int sc = kLandmarkSprite[spaceIdx][0], sr = kLandmarkSprite[spaceIdx][1];
        if (sc >= 0) {
            // 퀘벡(13) 랜드마크는 스프라이트가 오른쪽으로 치우쳐 있어 왼쪽 보정
            qreal offsetX = (spaceIdx == 13) ? -s * 0.35 : 0.0;
            QRectF dst(cx + offsetX - s * 1.1, cy - s * 1.1, s * 2.2, s * 2.2);
            drawSprite(p, sc, sr, dst, playerColor);
            return;
        }
    }
    // QPainter fallback (islands etc.) ───────────────────────────────────────
    p.setPen(QPen(col.darker(140), 1.2));
    p.setBrush(col.lighter(150));
    switch (spaceIdx) {
    case 1: { // 방콕 에메랄드 사원 - tiered temple
        for (int t = 0; t < 3; ++t) {
            qreal tw = s*(1.4-t*0.35), th = s*0.28, ty = cy+s*0.65-t*s*0.52;
            p.fillRect(QRectF(cx-tw/2, ty-th, tw, th), col.darker(105+t*12));
            QPolygonF rf; rf<<QPointF(cx-tw/2-s*.08,ty-th)<<QPointF(cx,ty-th-s*(.22-t*.02))<<QPointF(cx+tw/2+s*.08,ty-th);
            p.setBrush(col.lighter(125-t*8)); p.setPen(QPen(col.darker(130),0.8)); p.drawPolygon(rf);
        }
        p.setBrush(QColor(255,215,0)); p.setPen(QPen(col.darker(150),0.8));
        p.drawEllipse(QPointF(cx, cy-s*0.65), s*0.09, s*0.18);
        break;
    }
    case 3: { // 베이징 자금성 - gate with tower
        p.fillRect(QRectF(cx-s*.8, cy+s*.1, s*1.6, s*.75), QColor(180,40,40));
        p.fillRect(QRectF(cx-s*.48, cy-s*.58, s*.96, s*.72), col.darker(108));
        QPolygonF rf; rf<<QPointF(cx-s*.6,cy-s*.58)<<QPointF(cx,cy-s*1.05)<<QPointF(cx+s*.6,cy-s*.58);
        p.setBrush(QColor(50,100,50)); p.drawPolygon(rf);
        p.setBrush(Qt::black); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx, cy+s*.42), s*.14, s*.2);
        p.setBrush(Qt::NoBrush); p.setPen(QPen(col.darker(140),0.9));
        for (int w=-1;w<=1;++w) p.drawRect(QRectF(cx+w*s*.28-s*.06,cy-s*.42,s*.12,s*.2));
        break;
    }
    case 5: { // 타이페이 101 - segmented tower
        for (int sg=0;sg<8;++sg) {
            qreal sw=s*(.52-sg*.04), sh=s*.17, sy=cy+s*.75-sg*sh*1.05;
            p.fillRect(QRectF(cx-sw/2,sy-sh,sw,sh), col.darker(100+sg*5));
            p.setPen(QPen(col.darker(160),.5)); p.drawRect(QRectF(cx-sw/2,sy-sh,sw,sh));
        }
        p.setBrush(col.lighter(140)); p.setPen(QPen(col.darker(130),.8));
        p.drawRect(QRectF(cx-s*.05,cy-s*.65,s*.1,s*.18));
        break;
    }
    case 6: { // 두바이 부르즈 알 아랍 - sail
        QPainterPath sail;
        sail.moveTo(cx-s*.05, cy+s*.85);
        sail.quadTo(cx-s*.88, cy, cx-s*.08, cy-s*.9);
        sail.lineTo(cx+s*.05, cy-s*.9);
        sail.quadTo(cx+s*.48, cy-s*.18, cx+s*.05, cy+s*.85);
        sail.closeSubpath();
        p.fillPath(sail, col.lighter(150)); p.setPen(QPen(col.darker(130),1)); p.drawPath(sail);
        p.setBrush(col); p.setPen(QPen(col.darker(150),.8));
        p.drawEllipse(QPointF(cx, cy-s*.95), s*.14, s*.06);
        break;
    }
    case 7: { // 카이로 스핑크스
        p.setBrush(QColor(210,180,100)); p.setPen(QPen(QColor(160,130,60),1));
        p.drawRect(QRectF(cx-s*.8, cy, s*1.6, s*.55));
        p.drawEllipse(QPointF(cx+s*.55, cy-s*.1), s*.27, s*.3);
        QPolygonF hd; hd<<QPointF(cx+s*.28,cy-s*.36)<<QPointF(cx+s*.28,cy+s*.12)<<QPointF(cx+s*.16,cy+s*.12);
        p.drawPolygon(hd);
        p.drawRect(QRectF(cx-s*.8, cy+s*.5, s*.42, s*.16));
        break;
    }
    case 9: { // 발리 우붓 사원 - split gate
        for (int sd : {-1,1}) {
            qreal bx = cx+sd*s*.34;
            p.fillRect(QRectF(bx-s*.18,cy-s*.48,s*.36,s*1.25), col.darker(105));
            QPolygonF t1; t1<<QPointF(bx-s*.22,cy-s*.48)<<QPointF(bx,cy-s*.92)<<QPointF(bx+s*.22,cy-s*.48);
            p.setBrush(col.darker(118)); p.drawPolygon(t1);
            QPolygonF t2; t2<<QPointF(bx-s*.15,cy-s*.68)<<QPointF(bx,cy-s*1.02)<<QPointF(bx+s*.15,cy-s*.68);
            p.drawPolygon(t2);
        }
        p.setBrush(col.lighter(130)); p.setPen(Qt::NoPen);
        for (int st=0;st<3;++st) p.fillRect(QRectF(cx-s*(.1+st*.07),cy+s*(.58+st*.14),s*(.2+st*.14),s*.13), col.lighter(120+st*10));
        break;
    }
    case 10: { // 도쿄 타워
        p.setPen(QPen(QColor(255,80,0),1.5));
        p.drawLine(QPointF(cx-s*.58,cy+s*.8),QPointF(cx,cy-s*1.0));
        p.drawLine(QPointF(cx+s*.58,cy+s*.8),QPointF(cx,cy-s*1.0));
        p.drawLine(QPointF(cx-s*.58,cy+s*.8),QPointF(cx-s*.25,cy+s*.1));
        p.drawLine(QPointF(cx+s*.58,cy+s*.8),QPointF(cx+s*.25,cy+s*.1));
        p.setPen(QPen(QColor(255,80,0),1.0));
        for (qreal ty : {cy+s*.48, cy+s*.08, cy-s*.35}) {
            qreal hw=(ty-(cy-s*1.0))/(cy+s*.8-(cy-s*1.0))*s*.58;
            p.drawLine(QPointF(cx-hw,ty),QPointF(cx+hw,ty));
        }
        p.setBrush(QColor(255,80,0)); p.setPen(Qt::NoPen);
        p.drawRect(QRectF(cx-s*.11,cy-s*.44,s*.22,s*.11));
        break;
    }
    case 11: { // 시드니 오페라 하우스 - shells
        p.setPen(QPen(col.darker(120),1.2));
        for (int sh=0;sh<3;++sh) {
            qreal ox=cx-s*.48+sh*s*.37, ht=s*(.92-sh*.14);
            QPainterPath shell;
            shell.moveTo(ox, cy+s*.28);
            shell.quadTo(ox+s*.04, cy-ht, ox+s*.33, cy+s*.28);
            p.fillPath(shell, col.lighter(138+sh*10)); p.drawPath(shell);
        }
        p.fillRect(QRectF(cx-s*.72,cy+s*.28,s*1.44,s*.14), QColor(220,210,190));
        break;
    }
    case 13: { // 퀘벡 샤토 프롱트낙 - castle
        p.fillRect(QRectF(cx-s*.58,cy-s*.18,s*1.16,s*.88), col.darker(105));
        QPolygonF mr; mr<<QPointF(cx-s*.58,cy-s*.18)<<QPointF(cx,cy-s*.72)<<QPointF(cx+s*.58,cy-s*.18);
        p.setBrush(col.darker(128)); p.drawPolygon(mr);
        for (int t : {-1,1}) {
            qreal tx=cx+t*s*.5;
            p.fillRect(QRectF(tx-s*.13,cy-s*.48,s*.26,s*1.18), col.darker(110));
            QPolygonF tr2; tr2<<QPointF(tx-s*.17,cy-s*.48)<<QPointF(tx,cy-s*.82)<<QPointF(tx+s*.17,cy-s*.48);
            p.setBrush(col.darker(148)); p.drawPolygon(tr2);
        }
        break;
    }
    case 14: { // 하와이 와이키키 해변
        p.fillRect(QRectF(cx-s*.8,cy+s*.14,s*1.6,s*.52), QColor(100,180,220));
        p.fillRect(QRectF(cx-s*.8,cy+s*.48,s*1.6,s*.28), QColor(240,220,160));
        p.setPen(QPen(QColor(60,140,200),1.2));
        for (qreal wx=cx-s*.58; wx<cx+s*.48; wx+=s*.28)
            p.drawArc(QRectF(wx,cy+s*.18,s*.24,s*.14),0,180*16);
        p.setPen(QPen(QColor(139,90,43),2));
        p.drawLine(QPointF(cx+s*.38,cy+s*.44),QPointF(cx+s*.32,cy-s*.52));
        p.setPen(QPen(QColor(30,130,30),1.5));
        for (int lf=0;lf<4;++lf) {
            double a=-0.6+lf*.45;
            p.drawLine(QPointF(cx+s*.32,cy-s*.52),QPointF(cx+s*.32+std::cos(a)*s*.48, cy-s*.52+std::sin(a)*s*.33));
        }
        break;
    }
    case 15: { // 상파울루 대성당 - twin towers
        for (int t : {-1,1}) {
            qreal tx=cx+t*s*.3;
            p.fillRect(QRectF(tx-s*.17,cy-s*.68,s*.34,s*1.38), col.darker(103+t*4));
            QPolygonF sp; sp<<QPointF(tx-s*.17,cy-s*.68)<<QPointF(tx,cy-s*1.02)<<QPointF(tx+s*.17,cy-s*.68);
            p.setBrush(col.darker(128)); p.drawPolygon(sp);
        }
        p.fillRect(QRectF(cx-s*.48,cy,s*.96,s*.68), col.lighter(108));
        p.setBrush(Qt::NoBrush); p.setPen(QPen(col.darker(148),1));
        p.drawEllipse(QPointF(cx,cy+s*.12),s*.15,s*.2);
        break;
    }
    case 17: { // 프라하 성 - castle wall
        p.fillRect(QRectF(cx-s*.72,cy-s*.08,s*1.44,s*.78), col.darker(105));
        for (int m=-2;m<=2;++m)
            p.fillRect(QRectF(cx+m*s*.27-s*.08,cy-s*.26,s*.16,s*.18), col.darker(128));
        p.fillRect(QRectF(cx-s*.2,cy-s*.62,s*.4,s*.54), col.darker(108));
        QPolygonF ct; ct<<QPointF(cx-s*.24,cy-s*.62)<<QPointF(cx,cy-s*.98)<<QPointF(cx+s*.24,cy-s*.62);
        p.setBrush(col.darker(138)); p.drawPolygon(ct);
        break;
    }
    case 18: { // 푸켓 왓 찰롱 - thai spire
        for (int t=0;t<4;++t) {
            qreal tw=s*(.44-t*.08), th=s*.2, ty=cy+s*.58-t*s*.38;
            p.fillRect(QRectF(cx-tw,ty-th,tw*2,th), col.darker(104+t*10));
            QPolygonF r3; r3<<QPointF(cx-tw-s*.05,ty-th)<<QPointF(cx,ty-th-s*.16)<<QPointF(cx+tw+s*.05,ty-th);
            p.setBrush(col.darker(118+t*10)); p.drawPolygon(r3);
        }
        p.setBrush(QColor(255,215,0)); p.drawEllipse(QPointF(cx,cy-s*.98),s*.07,s*.1);
        break;
    }
    case 19: { // 베를린 브란덴부르크 문 - 5 columns + pediment
        for (int c2=-2;c2<=2;++c2)
            p.fillRect(QRectF(cx+c2*s*.27-s*.07,cy-s*.52,s*.14,s*1.18), col.darker(103+qAbs(c2)*8));
        p.fillRect(QRectF(cx-s*.68,cy-s*.58,s*1.36,s*.1), col.darker(118));
        QPolygonF qd; qd<<QPointF(cx-s*.52,cy-s*.68)<<QPointF(cx,cy-s*.96)<<QPointF(cx+s*.52,cy-s*.68);
        p.setBrush(col.darker(108)); p.drawPolygon(qd);
        break;
    }
    case 21: { // 모스크바 성 바실리 - 5 onion domes
        const qreal dX[5]={0,-s*.52,s*.52,-s*.28,s*.28};
        const qreal dY[5]={cy-s*.28,cy-s*.05,cy-s*.05,cy,cy};
        const qreal dH[5]={s*.62,s*.44,s*.44,s*.33,s*.33};
        const QColor dc[5]={{200,30,30},{30,100,200},{30,200,30},{200,150,30},{150,30,200}};
        p.fillRect(QRectF(cx-s*.68,cy+s*.38,s*1.36,s*.38), col.darker(108));
        for (int d=0;d<5;++d) {
            qreal dx2=cx+dX[d], dy=dY[d], dh=dH[d], dw=dh*.5;
            p.fillRect(QRectF(dx2-dw*.32,dy,dw*.64,s*.22), col.darker(118));
            QPainterPath dp;
            dp.moveTo(dx2-dw/2,dy);
            dp.quadTo(dx2-dw*.68,dy-dh*.5,dx2,dy-dh);
            dp.quadTo(dx2+dw*.68,dy-dh*.5,dx2+dw/2,dy);
            p.fillPath(dp,dc[d]); p.setPen(QPen(dc[d].darker(128),.8)); p.drawPath(dp);
        }
        break;
    }
    case 22: { // 제네바 제트 도 - water jet
        p.fillRect(QRectF(cx-s*.78,cy+s*.38,s*1.56,s*.33), QColor(100,160,220));
        QPainterPath jet;
        jet.moveTo(cx-s*.06,cy+s*.38);
        jet.quadTo(cx-s*.11,cy-s*.48,cx,cy-s*1.02);
        jet.quadTo(cx+s*.11,cy-s*.48,cx+s*.06,cy+s*.38);
        p.fillPath(jet,QColor(180,220,255,200));
        p.setPen(QPen(QColor(100,160,220),.8)); p.drawPath(jet);
        p.setBrush(QColor(200,235,255,160)); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx,cy-s*1.02),s*.14,s*.09);
        break;
    }
    case 23: { // 로마 콜로세움 - arched oval
        p.setPen(QPen(col.darker(128),1.5)); p.setBrush(col.lighter(140));
        p.drawEllipse(QPointF(cx,cy),s*.72,s*.52);
        p.setBrush(QColor(30,30,30,110));
        p.drawEllipse(QPointF(cx,cy),s*.42,s*.26);
        p.setBrush(Qt::NoBrush); p.setPen(QPen(col.darker(138),1));
        for (int a=0;a<8;++a) {
            double ang=a*M_PI/4.0;
            p.drawEllipse(QPointF(cx+std::cos(ang)*s*.64,cy+std::sin(ang)*s*.44),s*.07,s*.09);
        }
        break;
    }
    case 25: { // 타히티 수상 방갈로
        p.fillRect(QRectF(cx-s*.78,cy+s*.33,s*1.56,s*.28), QColor(100,180,220));
        p.setPen(QPen(QColor(139,90,43),1.5));
        for (qreal sx2 : {cx-s*.32,cx,cx+s*.32}) p.drawLine(QPointF(sx2,cy+s*.33),QPointF(sx2,cy+s*.58));
        p.fillRect(QRectF(cx-s*.52,cy-s*.02,s*1.04,s*.34), QColor(210,175,120));
        p.fillRect(QRectF(cx-s*.36,cy-s*.24,s*.72,s*.26), col.lighter(128));
        QPolygonF rf; rf<<QPointF(cx-s*.46,cy-s*.24)<<QPointF(cx,cy-s*.68)<<QPointF(cx+s*.46,cy-s*.24);
        p.setBrush(QColor(160,100,40)); p.drawPolygon(rf);
        break;
    }
    case 26: { // 런던 빅 벤
        p.fillRect(QRectF(cx-s*.21,cy-s*.62,s*.42,s*1.3), col.darker(105));
        p.fillRect(QRectF(cx-s*.27,cy-s*.72,s*.54,s*.14), col.darker(118));
        p.fillRect(QRectF(cx-s*.17,cy-s*.86,s*.34,s*.15), col.darker(128));
        QPolygonF tp; tp<<QPointF(cx-s*.19,cy-s*.86)<<QPointF(cx,cy-s*1.12)<<QPointF(cx+s*.19,cy-s*.86);
        p.setBrush(col.darker(138)); p.drawPolygon(tp);
        p.setBrush(Qt::white); p.setPen(QPen(col.darker(138),.8));
        p.drawEllipse(QPointF(cx,cy-s*.34),s*.15,s*.15);
        p.setPen(QPen(col.darker(158),1));
        p.drawLine(QPointF(cx,cy-s*.34),QPointF(cx,cy-s*.34-s*.11));
        p.drawLine(QPointF(cx,cy-s*.34),QPointF(cx+s*.08,cy-s*.34));
        break;
    }
    case 27: { // 파리 에펠탑
        p.setPen(QPen(col.darker(138),1.5));
        p.drawLine(QPointF(cx-s*.62,cy+s*.82),QPointF(cx,cy-s*.98));
        p.drawLine(QPointF(cx+s*.62,cy+s*.82),QPointF(cx,cy-s*.98));
        p.drawLine(QPointF(cx-s*.62,cy+s*.82),QPointF(cx-s*.27,cy+s*.06));
        p.drawLine(QPointF(cx+s*.62,cy+s*.82),QPointF(cx+s*.27,cy+s*.06));
        p.setPen(QPen(col.darker(128),1.0));
        p.drawLine(QPointF(cx-s*.5,cy+s*.48),QPointF(cx+s*.5,cy+s*.48));
        p.drawLine(QPointF(cx-s*.27,cy+s*.06),QPointF(cx+s*.27,cy+s*.06));
        p.drawLine(QPointF(cx-s*.14,cy-s*.32),QPointF(cx+s*.14,cy-s*.32));
        break;
    }
    case 29: { // 뉴욕 자유의 여신상
        p.fillRect(QRectF(cx-s*.28,cy+s*.28,s*.56,s*.42), QColor(150,150,160));
        p.fillRect(QRectF(cx-s*.17,cy-s*.12,s*.34,s*.48), col.darker(105));
        p.drawEllipse(QPointF(cx,cy-s*.25),s*.15,s*.17);
        p.setPen(QPen(col.darker(128),1));
        for (int sp2=-2;sp2<=2;++sp2) p.drawLine(QPointF(cx+sp2*s*.06,cy-s*.42),QPointF(cx+sp2*s*.09,cy-s*.6));
        p.setPen(QPen(col.darker(118),1.5));
        p.drawLine(QPointF(cx+s*.17,cy-s*.08),QPointF(cx+s*.4,cy-s*.66));
        p.setBrush(QColor(255,200,50)); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx+s*.4,cy-s*.72),s*.07,s*.1);
        break;
    }
    case 31: { // 서울 63빌딩
        p.setBrush(QColor(255,215,0,200)); p.setPen(QPen(QColor(180,140,0),1));
        p.drawRect(QRectF(cx-s*.26,cy-s*.98,s*.52,s*1.65));
        p.setBrush(Qt::NoBrush); p.setPen(QPen(QColor(200,160,0,120),.6));
        for (int fl=0;fl<11;++fl)
            p.drawLine(QPointF(cx-s*.26,cy-s*.92+fl*s*.15),QPointF(cx+s*.26,cy-s*.92+fl*s*.15));
        QPolygonF tp; tp<<QPointF(cx-s*.26,cy-s*.98)<<QPointF(cx,cy-s*1.18)<<QPointF(cx+s*.26,cy-s*.98);
        p.setBrush(QColor(255,215,0)); p.setPen(QPen(QColor(180,140,0),.8)); p.drawPolygon(tp);
        break;
    }
    default: { // generic star
        p.setBrush(col); p.setPen(QPen(col.darker(138),1));
        QPolygonF star;
        for (int k=0;k<10;++k) {
            double angle=k*M_PI/5.0-M_PI/2.0;
            double r2=(k%2==0)?s:s*.42;
            star<<QPointF(cx+std::cos(angle)*r2, cy+std::sin(angle)*r2);
        }
        p.drawPolygon(star);
    }
    }
}

// Draws a single building icon — sprite first, QPainter fallback.
// Painter is already translated to cell center + rotated.
static void paintBuildingIcon(QPainter& p, const QRectF& local, int buildingIdx, bool island, QColor col, int spaceIdx = -1, int playerColor = -1) {
    QRectF dst = iconDst(local);
    const bool hasSheet = !spriteSheet(playerColor).isNull();

    if (hasSheet) {
        if (buildingIdx == 4) {
            qreal ls = qMin(local.width(), local.height()) * 0.50;
            drawLandmarkIcon(p, 0, -local.height() * 0.05, ls, spaceIdx, col, playerColor);
            return;
        }
        int sprCol = -1;
        if      (buildingIdx == 0)            sprCol = 0; // 깃발
        else if (buildingIdx == 1 && island)  sprCol = 1; // 파라솔
        else if (buildingIdx == 1 && !island) sprCol = 3; // 빌라
        else if (buildingIdx == 2 && island)  sprCol = 2; // 멋진숙소
        else if (buildingIdx == 2 && !island) sprCol = 4; // 빌딩
        else if (buildingIdx == 3)            sprCol = 5; // 호텔
        if (sprCol >= 0) { drawSprite(p, sprCol, 0, dst, playerColor); return; }
    }

    // QPainter fallback ────────────────────────────────────────────
    const qreal s = qMin(local.width(), local.height()) * 0.30;
    const qreal cx = 0, cy = -local.height() * 0.15;
    p.setPen(QPen(col.darker(130), 1.2));
    p.setBrush(col.lighter(160));
    switch (buildingIdx) {
    case 0: {
        p.setBrush(col);
        p.drawLine(QPointF(cx,cy+s*.1),QPointF(cx,cy+s*1.6));
        QPolygonF f; f<<QPointF(cx,cy+s*.1)<<QPointF(cx+s*.9,cy+s*.55)<<QPointF(cx,cy+s);
        p.drawPolygon(f); break;
    }
    case 1: {
        if (island) { p.setBrush(col.lighter(150)); p.drawEllipse(QPointF(cx,cy-s*.2),s,s*.45); p.drawLine(QPointF(cx,cy-s*.2),QPointF(cx+s*.4,cy+s)); }
        else { p.drawRect(QRectF(cx-s*.6,cy-s*.2,s*1.2,s)); QPolygonF r; r<<QPointF(cx-s*.75,cy-s*.2)<<QPointF(cx,cy-s)<<QPointF(cx+s*.75,cy-s*.2); p.drawPolygon(r); }
        break;
    }
    case 2: {
        if (island) { p.drawRect(QRectF(cx-s*.55,cy-s*.1,s*1.1,s*.8)); QPolygonF r; r<<QPointF(cx-s*.65,cy-s*.1)<<QPointF(cx,cy-s*.85)<<QPointF(cx+s*.65,cy-s*.1); p.drawPolygon(r); }
        else { p.drawRect(QRectF(cx-s*.4,cy-s,s*.8,s*1.3)); p.setPen(QPen(col.darker(150),.6)); for(int row=0;row<3;++row) for(int c2=0;c2<2;++c2) p.drawRect(QRectF(cx-s*.28+c2*s*.32,cy-s*.85+row*s*.35,s*.18,s*.18)); }
        break;
    }
    case 3: {
        p.setBrush(col.lighter(138)); p.setPen(QPen(col.darker(138),1));
        p.drawRect(QRectF(cx-s*.82,cy-s*.95,s*1.64,s*1.72));
        p.fillRect(QRectF(cx-s*.88,cy-s*1.0,s*1.76,s*.1),col.darker(125));
        p.setBrush(QColor(200,230,255,180)); p.setPen(QPen(col.darker(155),.5));
        for(int row=0;row<3;++row) for(int c2=0;c2<3;++c2) p.drawRect(QRectF(cx-s*.62+c2*s*.48,cy-s*.82+row*s*.5,s*.26,s*.32));
        p.setBrush(col.darker(108)); p.setPen(QPen(col.darker(145),1));
        QPolygonF aw; aw<<QPointF(cx-s*.42,cy+s*.58)<<QPointF(cx,cy+s*.32)<<QPointF(cx+s*.42,cy+s*.58); p.drawPolygon(aw);
        p.setBrush(QColor(255,215,0)); p.setPen(QPen(QColor(160,120,0),1));
        p.drawRect(QRectF(cx-s*.52,cy-s*1.14,s*1.04,s*.2));
        QFont f; f.setPixelSize(qMax(5.0,s*.16)); f.setBold(true);
        p.setFont(f); p.setPen(QColor(120,70,0));
        p.drawText(QRectF(cx-s*.52,cy-s*1.14,s*1.04,s*.2),Qt::AlignCenter,"HOTEL");
        break;
    }
    case 4: {
        qreal ls = qMin(local.width(), local.height()) * 0.40;
        drawLandmarkIcon(p, cx, cy, ls, spaceIdx, col);
        break;
    }
    }
}

} // namespace

void EditorCanvas::drawBuildings(QPainter& p, const CellState* states, bool allDown) {
    if (!states) return;
    p.setRenderHint(QPainter::Antialiasing);

    const int kRot[4]      = {0, 90, 180, -90};
    const int cornerRot[4] = {-45, 45, 135, -135};

    for (int i = 0; i < kBoardTotal; ++i) {
        const auto& cs = states[i];
        if (cs.playerColor < 0) continue;

        // Collect active slots
        QList<int> active;
        for (int s = 0; s < 5; ++s)
            if (cs.buildingCounts[s] > 0) active.append(s);
        if (active.isEmpty()) continue;

        QRectF r = boardSpaceRect(i);
        int rot = (i % 8 == 0) ? cornerRot[i / 8] : (allDown ? 0 : kRot[i / 8]);

        p.save();
        p.translate(r.center());
        p.rotate(rot);
        bool is90 = (rot == 90 || rot == -90 || rot == -135 || rot == 135);
        QRectF local(-(is90 ? r.height() : r.width())/2,
                     -(is90 ? r.width()  : r.height())/2,
                      (is90 ? r.height() : r.width()),
                      (is90 ? r.width()  : r.height()));

        QColor col = kPlayerColors[cs.playerColor];
        int n = active.size();

        // Layout: offsets (dx,dy) as fraction of local half-dims, plus scale
        struct Sub { qreal dx, dy, sc; };
        qreal hw = local.width()*0.5, hh = local.height()*0.5;
        QList<Sub> layout;
        switch (n) {
        case 1: layout = {{0,0,1.0}}; break;
        case 2: layout = {{-hw*.42,0,.62},{hw*.42,0,.62}}; break;
        case 3: layout = {{-hw*.42,-hh*.35,.56},{hw*.42,-hh*.35,.56},{0,hh*.38,.56}}; break;
        case 4: layout = {{-hw*.38,-hh*.3,.5},{hw*.38,-hh*.3,.5},{-hw*.38,hh*.3,.5},{hw*.38,hh*.3,.5}}; break;
        default:layout = {{-hw*.42,-hh*.32,.44},{0,-hh*.32,.44},{hw*.42,-hh*.32,.44},{-hw*.26,hh*.3,.44},{hw*.26,hh*.3,.44}}; break;
        }

        for (int k = 0; k < n; ++k) {
            int slot = active[k];
            const Sub& sub = layout[k];
            p.save();
            p.translate(sub.dx, sub.dy);
            p.scale(sub.sc, sub.sc);
            qreal inv = 1.0 / sub.sc;
            QRectF subLocal(local.left()*inv, local.top()*inv, local.width()*inv, local.height()*inv);
            paintBuildingIcon(p, subLocal, slot, isIslandCell(i), col, i, cs.playerColor);
            p.restore();
        }

        // 하단 도시명 배지
        if (i % 8 != 0) {
            const QString& name = kBoardSpaces[i].name;
            const QColor& cellColor = kBoardSpaces[i].color;
            int fs = qMax(8, qMin(13, (int)(qMin(local.width(), local.height()) * 0.20)));
            QFont nf("맑은 고딕"); nf.setPixelSize(fs); nf.setBold(true);
            nf.setStyleStrategy(QFont::PreferAntialias);
            p.setFont(nf);
            QFontMetrics fm(nf);
            qreal tw = fm.horizontalAdvance(name);
            qreal th = fm.height();
            qreal nx = -tw * 0.5;
            qreal ny = local.bottom() - th - 2;
            QColor bg = cellColor; bg.setAlpha(230);
            p.setBrush(bg); p.setPen(Qt::NoPen);
            p.drawRoundedRect(QRectF(nx - 4, ny - 1, tw + 8, th + 2), 3, 3);
            p.setPen(cellColor.lightness() < 155 ? Qt::white : QColor(30,30,30));
            p.drawText(QPointF(nx, ny + fm.ascent()), name);
        }

        p.restore();
    }
}
