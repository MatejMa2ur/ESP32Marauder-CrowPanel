#pragma once

#include "configs.h"

#ifdef MARAUDER_CROWPANEL_ADV

#include "CrowPanelDisplay.h"
#include "CrowPanelLayout.h"

// ---------------------------------------------------------------------------
// Marauder-GD style widgets for the CrowPanel 5.0" Advance: flat, radius-3
// rounded rects, 1px EDGE borders, black background, accent only on outlines
// and primary text. Geometry/palette live in CrowPanelLayout.h; this file only
// draws. Every function restores font/textsize/datum so Marauder's own
// drawing is unaffected.
// ---------------------------------------------------------------------------

namespace crowpanel {
namespace ui {

enum class Style : uint8_t { Neutral, Primary, Danger, Disabled };

constexpr int16_t kRadius = 3;

inline void restore(CrowPanelDisplay &tft) {
  tft.setFont(nullptr);
  tft.setTextSize(1);
  tft.setTextDatum(lgfx::top_left);
}

// Copy `s` into `out`, appending "..." if it is wider than maxW in the
// currently selected font.
inline void fit(CrowPanelDisplay &tft, const char *s, int16_t maxW, char *out, size_t n) {
  strlcpy(out, s ? s : "", n);
  if (tft.textWidth(out) <= maxW) return;
  size_t len = strlen(out);
  while (len > 0) {
    len--;
    snprintf(out, n, "%.*s...", (int)len, s);
    if (tft.textWidth(out) <= maxW) return;
  }
}

inline void drawButton(CrowPanelDisplay &tft, Rect r, const char *label, Style s, bool pressed = false) {
  uint16_t edge = kEdge, text = kText, fill = kPanel;
  switch (s) {
    case Style::Primary:  edge = text = kAccent; break;
    case Style::Danger:   edge = text = kDanger; break;
    case Style::Disabled: fill = kBg; text = kMuted; break;
    default: break;
  }
  if (pressed) fill = kPressed;
  tft.fillRoundRect(r.x, r.y, r.w, r.h, kRadius, fill);
  tft.drawRoundRect(r.x, r.y, r.w, r.h, kRadius, edge);
  tft.setFont(&fonts::FreeSans12pt7b);
  tft.setTextColor(text, fill);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
  restore(tft);
}

inline void drawHeader(CrowPanelDisplay &tft, const char *title, const char *subtitle,
                       bool showBack, bool stopActive) {
  tft.fillRect(0, 0, 800, kHeaderH, kBg);
  char buf[64];
  tft.setFont(&fonts::FreeSansBold12pt7b);
  fit(tft, title, 540 - kTitleX, buf, sizeof(buf));
  tft.setTextColor(kText, kBg);
  tft.drawString(buf, kTitleX, kTitleY);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kMuted, kBg);
  tft.drawString(subtitle ? subtitle : "", kSubtitleX, kSubtitleY);
  restore(tft);
  if (showBack) drawButton(tft, kBackBtn, "BACK", Style::Neutral);
  drawButton(tft, kStopBtn, "STOP", stopActive ? Style::Danger : Style::Disabled);
  tft.drawFastHLine(0, kHeaderH, 800, kEdge);
}

inline void drawRow(CrowPanelDisplay &tft, uint8_t i, const char *label, uint16_t color,
                    const uint8_t *icon, bool toggle, bool checked, bool pressed) {
  Rect r = rowRect(i);
  uint16_t fill = pressed ? kPressed : kPanel;
  tft.fillRoundRect(r.x, r.y, r.w, r.h, kRadius, fill);
  tft.drawRoundRect(r.x, r.y, r.w, r.h, kRadius, kEdge);
  if (icon) tft.drawXBitmap(r.x + 12, r.y + (r.h - 22) / 2, icon, 22, 22, color);

  const int16_t right = r.x + r.w - 16;  // toggle/chevron sit inside this margin
  char buf[48];
  tft.setFont(&fonts::FreeSans12pt7b);
  fit(tft, label, right - (toggle ? 24 : 12) - (r.x + 48) - 8, buf, sizeof(buf));
  tft.setTextColor(kText, fill);
  tft.setTextDatum(lgfx::middle_left);
  tft.drawString(buf, r.x + 48, r.y + r.h / 2);
  restore(tft);

  const int16_t cy = r.y + r.h / 2;
  if (toggle) {
    const int16_t bx = right - 24, by = cy - 12;
    if (checked) {
      tft.fillRoundRect(bx, by, 24, 24, 2, kAccent);
      tft.drawLine(bx + 5, by + 12, bx + 10, by + 17, kBg);
      tft.drawLine(bx + 5, by + 13, bx + 10, by + 18, kBg);
      tft.drawLine(bx + 10, by + 17, bx + 19, by + 7, kBg);
      tft.drawLine(bx + 10, by + 18, bx + 19, by + 8, kBg);
    } else {
      tft.drawRoundRect(bx, by, 24, 24, 2, kEdge);
    }
  } else {
    const int16_t cx = right - 6;
    tft.drawLine(cx - 4, cy - 8, cx + 2, cy, kMuted);
    tft.drawLine(cx + 2, cy, cx - 4, cy + 8, kMuted);
    tft.drawLine(cx - 3, cy - 8, cx + 3, cy, kMuted);
    tft.drawLine(cx + 3, cy, cx - 3, cy + 8, kMuted);
  }
}

inline void clearRow(CrowPanelDisplay &tft, uint8_t i) {
  Rect r = rowRect(i);
  tft.fillRect(r.x, r.y, r.w, r.h, kBg);
}

inline void drawFooter(CrowPanelDisplay &tft, uint16_t page, uint16_t pages) {
  tft.fillRect(0, 400, 800, 56, kBg);
  if (pages <= 1) return;
  drawButton(tft, kPrevBtn, "< PREV", page > 0 ? Style::Neutral : Style::Disabled);
  drawButton(tft, kNextBtn, "NEXT >", page + 1 < pages ? Style::Neutral : Style::Disabled);
  char buf[24];
  snprintf(buf, sizeof(buf), "page %u/%u", (unsigned)page + 1, (unsigned)pages);  // page is 0-based
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kMuted, kBg);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(buf, kPageLabelX, kPageLabelY);
  restore(tft);
}

inline void drawStatusLine(CrowPanelDisplay &tft, const char *text) {
  tft.fillRect(0, kStatusY - 2, 800, 480 - (kStatusY - 2), kBg);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kMuted, kBg);
  tft.drawString(text ? text : "", 18, kStatusY);
  restore(tft);
}

inline void drawToolbar(CrowPanelDisplay &tft, uint8_t channel, bool hopOn) {
  drawButton(tft, kChDownBtn, "CH -", Style::Neutral);
  drawButton(tft, kChUpBtn, "CH +", Style::Neutral);
  drawButton(tft, kHopBtn, "HOP", hopOn ? Style::Primary : Style::Neutral);
  char buf[12];
  snprintf(buf, sizeof(buf), "ch %u", (unsigned)channel);
  tft.fillRect(kChLabel.x, kChLabel.y, kChLabel.w, kChLabel.h, kBg);
  tft.setFont(&fonts::FreeSans12pt7b);
  tft.setTextColor(kText, kBg);
  tft.setTextDatum(lgfx::middle_center);
  tft.drawString(buf, kChLabel.x + kChLabel.w / 2, kChLabel.y + kChLabel.h / 2);
  restore(tft);
}

inline void clearBody(CrowPanelDisplay &tft, int16_t top) {
  tft.fillRect(0, top, 800, 480 - top, kBg);
}

}  // namespace ui
}  // namespace crowpanel

#endif  // MARAUDER_CROWPANEL_ADV
