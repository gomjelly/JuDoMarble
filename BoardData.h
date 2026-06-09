#pragma once
#include <QString>
#include <QColor>
#include <QRectF>

// ── Board constants ────────────────────────────────────────────────────────────
static constexpr qreal kBoardSize   = 630.0;
static constexpr qreal kCornerSize  = 90.0;
static constexpr int   kBoardSide   = 8;    // spaces per side (incl. starting corner)
static constexpr int   kBoardTotal  = 32;   // 4 sides × 8 = 32
static const     qreal kCellWidth   = (kBoardSize - 2.0 * kCornerSize) / 7.0; // ≈ 64.3

// ── Space types ────────────────────────────────────────────────────────────────
enum class SpaceType { Property, Corner, FortuneCard, Bonus };

struct BoardSpace {
    QString   name;
    QColor    color;
    SpaceType type;
};

// ── Space table ────────────────────────────────────────────────────────────────
// Clockwise order starting from index 0 = 출발 (BR corner).
// Side layout:   0 = right column  (BR→TR, going up)
//                1 = top row       (TR→TL, going left)
//                2 = left column   (TL→BL, going down)
//                3 = bottom row    (BL→BR, going right)
// Index % 8 == 0 → corner.  side = index / 8.
static const BoardSpace kBoardSpaces[kBoardTotal] = {
    // ── side 0: bottom row (BR → BL) — pastel-blended colors ──
    {"출발",      {101,184,101}, SpaceType::Corner},     //  0  BR
    {"방콕",      {164,223,143}, SpaceType::Property},   //  1
    {"보너스게임",{139,221,139}, SpaceType::Bonus},      //  2
    {"베이징",    {164,223,143}, SpaceType::Property},   //  3
    {"독도",      {194,226,236}, SpaceType::Property},   //  4
    {"타이페이",  { 98,184,114}, SpaceType::Property},   //  5
    {"두바이",    { 98,184,114}, SpaceType::Property},   //  6
    {"카이로",    { 98,184,114}, SpaceType::Property},   //  7
    // ── side 1: left column (BL → TL) ──
    {"무인도",    {236,214,161}, SpaceType::Corner},     //  8  BL
    {"발리",      {255,227,227}, SpaceType::Property},   //  9
    {"도쿄",      {139,200,248}, SpaceType::Property},   // 10
    {"시드니",    {139,200,248}, SpaceType::Property},   // 11
    {"포춘카드",  {255,225, 64}, SpaceType::FortuneCard},// 12
    {"퀘벡",      { 86,139,214}, SpaceType::Property},   // 13
    {"하와이",    {194,226,236}, SpaceType::Property},   // 14
    {"상파울로",  { 86,139,214}, SpaceType::Property},   // 15
    // ── side 2: top row (TL → TR) ──
    {"올림픽",    {248,248,248}, SpaceType::Corner},     // 16  TL
    {"프라하",    {244,137,173}, SpaceType::Property},   // 17
    {"푸켓",      {194,226,236}, SpaceType::Property},   // 18
    {"베를린",    {244,137,173}, SpaceType::Property},   // 19
    {"포춘카드",  {255,225, 64}, SpaceType::FortuneCard},// 20
    {"모스크바",  {170,115,194}, SpaceType::Property},   // 21
    {"제네바",    {170,115,194}, SpaceType::Property},   // 22
    {"로마",      {170,115,194}, SpaceType::Property},   // 23
    // ── side 3: right column (TR → BR) ──
    {"세계여행",  { 83,116,191}, SpaceType::Corner},     // 24  TR
    {"타히티",    {255,227,227}, SpaceType::Property},   // 25
    {"런던",      {236,158, 89}, SpaceType::Property},   // 26
    {"파리",      {236,158, 89}, SpaceType::Property},   // 27
    {"포춘카드",  {255,225, 64}, SpaceType::FortuneCard},// 28
    {"뉴욕",      {221, 94, 94}, SpaceType::Property},   // 29
    {"국세청",    {109,109,109}, SpaceType::Property},   // 30
    {"서울",      {221, 94, 94}, SpaceType::Property},   // 31
};

// ── Space category helpers ────────────────────────────────────────────────────
// Islands (발리=9, 타히티=25): 깃발/파라솔/멋진숙소
// Cities (other Property): 깃발/빌라/빌딩/호텔/랜드마크
// Special (corners, fortune, bonus, 국세청): no buildings
inline bool isIslandCell(int idx) {
    return idx == 4 || idx == 9 || idx == 14 || idx == 18 || idx == 25; // 독도,발리,하와이,푸켓,타히티
}
inline bool canBuildHere(int idx) {
    const auto& sp = kBoardSpaces[idx];
    if (sp.type != SpaceType::Property) return false;
    if (sp.name == "국세청") return false;
    return true;
}
inline QStringList buildingNames(int idx) {
    if (!canBuildHere(idx)) return {};
    if (isIslandCell(idx)) return {"깃발", "파라솔", "멋진 숙소"};
    return {"깃발", "빌라", "빌딩", "호텔", "랜드마크"};
}

// ── boardSpaceRect ─────────────────────────────────────────────────────────────
// Returns the rect for space [idx] in canonical (0,0)‒(kBoardSize,kBoardSize) coords.
// Canonical corners: TL=(0,0), TR=(BS-CS,0), BR=(BS-CS,BS-CS), BL=(0,BS-CS)
//   index 0  (출발)    → BR corner
//   index 8  (무인도)  → TR corner
//   index 16 (올림픽)  → TL corner
//   index 24 (세계여행)→ BL corner
// Clockwise layout starting from 출발 (BR):
//   side 0 → bottom row,  BR→BL  (going left)
//   side 1 → left column, BL→TL  (going up)
//   side 2 → top row,     TL→TR  (going right)
//   side 3 → right column,TR→BR  (going down)
inline QRectF boardSpaceRect(int idx) {
    const qreal bs = kBoardSize;
    const qreal cs = kCornerSize;
    const qreal cw = kCellWidth;
    const int   side = idx / 8;
    const int   pos  = idx % 8;   // 0 = corner, 1‒7 = side cells

    if (pos == 0) {
        switch (side) {
            case 0: return {bs-cs, bs-cs, cs, cs}; // BR  출발
            case 1: return {0,     bs-cs, cs, cs}; // BL  무인도
            case 2: return {0,     0,     cs, cs}; // TL  올림픽
            case 3: return {bs-cs, 0,     cs, cs}; // TR  세계여행
        }
    }

    const int ci = pos - 1;  // cell index 0‒6 within side
    switch (side) {
        case 0: return {bs-cs-(ci+1)*cw, bs-cs, cw, cs}; // bottom, going left
        case 1: return {0, bs-cs-(ci+1)*cw,      cs, cw}; // left,   going up
        case 2: return {cs+ci*cw, 0,              cw, cs}; // top,    going right
        case 3: return {bs-cs, cs+ci*cw,          cs, cw}; // right,  going down
        default: return {};
    }
}
