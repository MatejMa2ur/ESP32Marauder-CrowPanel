#pragma once

#include <stdint.h>

namespace crowpanel {

// RGB565 palette
constexpr uint16_t kBg = 0x0000;
constexpr uint16_t kPanel = 0x0861;
constexpr uint16_t kEdge = 0x2945;
constexpr uint16_t kText = 0xE77C;
constexpr uint16_t kMuted = 0x9D73;
constexpr uint16_t kAccent = 0x72DF;
constexpr uint16_t kWarn = 0xF5ED;
constexpr uint16_t kDanger = 0xFBF0;
constexpr uint16_t kPressed = 0x18C7;

struct Rect {
  int16_t x, y, w, h;
};

// Half-open: x in [r.x, r.x+w), y in [r.y, r.y+h)
constexpr bool contains(Rect r, int16_t x, int16_t y) {
  return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

// Screen and header
constexpr int16_t kScreenW = 800;
constexpr int16_t kScreenH = 480;
constexpr int16_t kMargin = 18;
constexpr int16_t kHeaderH = 62;
constexpr int16_t kTitleX = 18;
constexpr int16_t kTitleY = 10;
constexpr int16_t kSubtitleX = 18;
constexpr int16_t kSubtitleY = 40;
constexpr Rect kBackBtn = {554, 8, 124, 44};
constexpr Rect kStopBtn = {686, 8, 96, 44};

// Menu rows
constexpr int16_t kBodyTop = 72;
constexpr uint8_t kRowsPerPage = 6;
constexpr int16_t kRowX = 18;
constexpr int16_t kRowW = 764;
constexpr int16_t kRowH = 48;
constexpr int16_t kRowPitch = 54;
constexpr int16_t kRowY0 = 72;

// Footer
constexpr Rect kPrevBtn = {18, 408, 120, 44};
constexpr Rect kNextBtn = {662, 408, 120, 44};
constexpr int16_t kPageLabelX = 400;
constexpr int16_t kPageLabelY = 430;
constexpr int16_t kStatusY = 461;

// Scan toolbar
constexpr Rect kChDownBtn = {18, 72, 96, 44};
constexpr Rect kChLabel = {124, 72, 96, 44};
constexpr Rect kChUpBtn = {230, 72, 96, 44};
constexpr Rect kHopBtn = {336, 72, 96, 44};
constexpr int16_t kScanBodyTop = 72;
constexpr int16_t kScanBodyTopToolbar = 124;
constexpr int16_t kTextLineH = 16;

enum class HeaderHit : uint8_t { None, Back, Stop };
enum class FooterHit : uint8_t { None, Prev, Next };
enum class ToolbarHit : uint8_t { None, ChDown, ChUp, Hop };

constexpr HeaderHit headerHit(int16_t x, int16_t y, bool backVisible) {
  return (backVisible && contains(kBackBtn, x, y)) ? HeaderHit::Back
         : contains(kStopBtn, x, y)                ? HeaderHit::Stop
                                                   : HeaderHit::None;
}

constexpr FooterHit footerHit(int16_t x, int16_t y, uint16_t pages) {
  return pages <= 1                 ? FooterHit::None
         : contains(kPrevBtn, x, y) ? FooterHit::Prev
         : contains(kNextBtn, x, y) ? FooterHit::Next
                                    : FooterHit::None;
}

constexpr Rect rowRect(uint8_t i) {
  return Rect{kRowX, static_cast<int16_t>(kRowY0 + i * kRowPitch), kRowW, kRowH};
}

// 0..kRowsPerPage-1 inside a row rect; gaps between rows and outside return -1
constexpr int8_t rowAt(int16_t x, int16_t y) {
  return (x < kRowX || x >= kRowX + kRowW || y < kRowY0 ||
          (y - kRowY0) / kRowPitch >= kRowsPerPage ||
          (y - kRowY0) % kRowPitch >= kRowH)
             ? -1
             : static_cast<int8_t>((y - kRowY0) / kRowPitch);
}

constexpr ToolbarHit toolbarHit(int16_t x, int16_t y) {
  return contains(kChDownBtn, x, y) ? ToolbarHit::ChDown
         : contains(kChUpBtn, x, y) ? ToolbarHit::ChUp
         : contains(kHopBtn, x, y)  ? ToolbarHit::Hop
                                    : ToolbarHit::None;
}

constexpr uint16_t pageCount(uint16_t items) {
  return items == 0 ? 1 : static_cast<uint16_t>((items + kRowsPerPage - 1) / kRowsPerPage);
}

constexpr uint16_t pageStart(uint16_t page) {
  return static_cast<uint16_t>(page * kRowsPerPage);
}

constexpr uint16_t pageOf(uint16_t index) {
  return static_cast<uint16_t>(index / kRowsPerPage);
}

constexpr uint16_t clampPage(int32_t page, uint16_t pages) {
  return page < 0 ? 0
         : (pages == 0 || page >= pages) ? static_cast<uint16_t>(pages == 0 ? 0 : pages - 1)
                                         : static_cast<uint16_t>(page);
}

constexpr uint8_t scanTextLines(bool toolbar) {
  return static_cast<uint8_t>((kScreenH - (toolbar ? kScanBodyTopToolbar : kScanBodyTop)) / kTextLineH);
}

}  // namespace crowpanel
