#pragma once

#include "configs.h"

#ifdef MARAUDER_CROWPANEL_ADV

#include "CrowPanelDisplay.h"
#include "CrowPanelLayout.h"

// ---------------------------------------------------------------------------
// Marauder-GD style widgets for the CrowPanel 5.0" Advance: flat, radius-3
// rounded rects, 1px EDGE borders, black background, accent only on outlines
// and primary text. Geometry/palette live in CrowPanelLayout.h; this file only
// draws. Every function resets font/textsize/datum so Marauder's own
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
  if (!s) s = "";
  strlcpy(out, s, n);
  if (tft.textWidth(out) <= maxW) return;
  size_t len = strlen(out);
  while (len > 0) {
    len--;
    snprintf(out, n, "%.*s...", (int)len, s);
    if (tft.textWidth(out) <= maxW) return;
  }
}

// --- Unified live scan/sniff view (shared by every WiFi/BLE scanner) ---------

// RSSI -> status colour (GD mapping): strong = green, mid = amber, weak = red.
inline uint16_t rssiColor(int rssi) {
  if (rssi >= -60) return 0x6F8D;  // ~#6ef36a green  "good"
  if (rssi >= -75) return kWarn;   // amber           "mid"
  return kDanger;                  // red             "weak"
}

// Banner under the stat cards: what the sniffer is doing + the channel it's on.
// Every scan view draws the same, so the user always sees mode + channel.
inline void drawBanner(CrowPanelDisplay &tft, int16_t top, const char *doing,
                       int channel, bool hopping) {
  Rect r = liveBanner(top);
  tft.fillRect(r.x, r.y, r.w, r.h, kBg);
  tft.fillRect(r.x, r.y + 3, 4, r.h - 6, kAccent);  // accent tick
  char buf[48];
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kText, kBg);
  tft.setTextDatum(lgfx::middle_left);
  fit(tft, doing ? doing : "", r.w - 200, buf, sizeof(buf));
  tft.drawString(buf, r.x + 14, r.y + r.h / 2);
  char meta[32];
  if (channel > 0)
    snprintf(meta, sizeof(meta), "ch %d%s", channel, hopping ? "  hop" : "");
  else
    snprintf(meta, sizeof(meta), "%s", hopping ? "hopping" : "");
  tft.setTextColor(kAccent, kBg);
  tft.setTextDatum(lgfx::middle_right);
  tft.drawString(meta, r.x + r.w - 6, r.y + r.h / 2);
  restore(tft);
}

// One result row: primary (name/SSID/MAC) left, meta (type/ch) muted, RSSI right
// colour-coded by strength. hasRssi=false hides the RSSI column.
inline void drawLiveRow(CrowPanelDisplay &tft, Rect r, const char *primary,
                        const char *meta, int rssi, bool hasRssi) {
  tft.fillRect(r.x, r.y, r.w, r.h, kBg);
  tft.drawFastHLine(r.x, r.y + r.h - 1, r.w, kEdge);  // thin separator
  char buf[64];
  tft.setFont(&fonts::Font2);
  int16_t text_right = r.x + r.w - 12;
  if (hasRssi) {
    char rb[12];
    snprintf(rb, sizeof(rb), "%d", rssi);
    tft.setTextColor(rssiColor(rssi), kBg);
    tft.setTextDatum(lgfx::middle_right);
    tft.drawString(rb, r.x + r.w - 12, r.y + r.h / 2);
    text_right = r.x + r.w - 72;
  }
  int16_t meta_w = 0;
  if (meta && meta[0]) {
    char mb[28];
    fit(tft, meta, 180, mb, sizeof(mb));
    meta_w = tft.textWidth(mb) + 16;
    tft.setTextColor(kMuted, kBg);
    tft.setTextDatum(lgfx::middle_right);
    tft.drawString(mb, text_right, r.y + r.h / 2);
  }
  tft.setTextColor(kText, kBg);
  tft.setTextDatum(lgfx::middle_left);
  fit(tft, primary ? primary : "", (text_right - meta_w) - (r.x + 12), buf, sizeof(buf));
  tft.drawString(buf, r.x + 12, r.y + r.h / 2);
  restore(tft);
}

// Clear the live-list body (banner top down to the footer).
inline void clearLiveBody(CrowPanelDisplay &tft, int16_t banner_top) {
  tft.fillRect(0, banner_top, kScreenW, kLiveFooterTop - banner_top, kBg);
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
  tft.fillRect(0, 0, crowpanel::kScreenW, kHeaderH, kBg);
  char buf[64];
  tft.setFont(&fonts::FreeSansBold12pt7b);
  const int16_t tx = kTitleX + 12;  // accent marker (GD "//" vibe) before the title
  tft.fillRect(kTitleX, kTitleY + 2, 4, 22, kAccent);
  fit(tft, title, (kBackBtn.x - 14) - tx, buf, sizeof(buf));
  tft.setTextColor(kText, kBg);
  tft.drawString(buf, tx, kTitleY);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kMuted, kBg);
  tft.drawString(subtitle ? subtitle : "", kSubtitleX, kSubtitleY);
  restore(tft);
  if (showBack) drawButton(tft, kBackBtn, "BACK", Style::Neutral);
  drawButton(tft, kStopBtn, "STOP", stopActive ? Style::Danger : Style::Disabled);
  tft.drawFastHLine(0, kHeaderH, crowpanel::kScreenW, kEdge);
}

inline void drawRow(CrowPanelDisplay &tft, uint8_t i, const char *label, uint16_t color,
                    const uint8_t *icon, bool toggle, bool checked, bool pressed) {
  Rect r = rowRect(i);
  uint16_t fill = pressed ? kPressed : kPanel;
  tft.fillRect(r.x, r.y, r.w, r.h, fill);
  tft.drawRect(r.x, r.y, r.w, r.h, kEdge);
  if (icon) tft.drawXBitmap(r.x + 12, r.y + (r.h - 22) / 2, icon, 22, 22, color);

  const int16_t right = r.x + r.w - 16;  // toggle/chevron sit inside this margin
  char buf[48];
  tft.setFont(&fonts::FreeSans12pt7b);
  // Label starts 30px clear of the 22px icon at r.x+12.
  fit(tft, label, right - (toggle ? 24 : 12) - (r.x + 64) - 8, buf, sizeof(buf));
  tft.setTextColor(kText, fill);
  tft.setTextDatum(lgfx::middle_left);
  tft.drawString(buf, r.x + 64, r.y + r.h / 2);
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

// Home tile: card with left accent strip, icon, title and one-line subtitle.
inline void drawTile(CrowPanelDisplay &tft, Rect r, const char *title, const char *subtitle,
                     uint16_t accent, const uint8_t *icon, bool pressed) {
  const uint16_t fill = pressed ? kPressed : kPanel;
  tft.fillRect(r.x, r.y, r.w, r.h, fill);
  tft.drawRect(r.x, r.y, r.w, r.h, kEdge);
  tft.fillRect(r.x, r.y, 4, r.h, accent);
  const int16_t x = r.x + 16, maxW = r.w - 32;
  if (icon) tft.drawXBitmap(x, r.y + 16, icon, 22, 22, accent);  // ponytail: 22x22, no x2 scaling
  char buf[48];
  tft.setTextDatum(lgfx::top_left);
  tft.setFont(&fonts::FreeSansBold12pt7b);
  fit(tft, title, maxW, buf, sizeof(buf));
  tft.setTextColor(kText, fill);
  tft.drawString(buf, x, r.y + 50);
  tft.setFont(&fonts::Font2);
  fit(tft, subtitle, maxW, buf, sizeof(buf));
  tft.setTextColor(kMuted, fill);
  tft.drawString(buf, x, r.y + 82);
  restore(tft);
}

// Value half of a stat card: clears only the value band (never the label), then draws.
// Compact (h < 56): label left, value right on the same row. Tall: value under the label.
inline void drawStatValue(CrowPanelDisplay &tft, Rect r, const char *value, uint16_t valueColor) {
  const bool compact = r.h < 56;
  char buf[32];
  tft.setFont(&fonts::FreeSansBold12pt7b);
  if (compact) {
    tft.fillRect(r.x + r.w / 2, r.y + 2, r.w / 2 - 2, r.h - 4, kPanel);
    fit(tft, value, r.w / 2 - 10, buf, sizeof(buf));
    tft.setTextDatum(lgfx::middle_right);
    tft.setTextColor(valueColor, kPanel);
    tft.drawString(buf, r.x + r.w - 10, r.y + r.h / 2);
  } else {
    tft.fillRect(r.x + 2, r.y + 28, r.w - 4, r.h - 30, kPanel);
    fit(tft, value, r.w - 20, buf, sizeof(buf));
    tft.setTextDatum(lgfx::bottom_left);
    tft.setTextColor(valueColor, kPanel);
    tft.drawString(buf, r.x + 10, r.y + r.h - 8);
  }
  restore(tft);
}

inline void drawStatCard(CrowPanelDisplay &tft, Rect r, const char *label, const char *value,
                         uint16_t valueColor) {
  tft.fillRect(r.x, r.y, r.w, r.h, kPanel);
  tft.drawRect(r.x, r.y, r.w, r.h, kEdge);
  const bool compact = r.h < 56;
  char buf[32];
  tft.setFont(&fonts::Font2);
  fit(tft, label, compact ? r.w / 2 - 10 : r.w - 20, buf, sizeof(buf));
  tft.setTextColor(kMuted, kPanel);
  tft.setTextDatum(compact ? lgfx::middle_left : lgfx::top_left);
  tft.drawString(buf, r.x + 10, compact ? r.y + r.h / 2 : r.y + 8);
  drawStatValue(tft, r, value, valueColor);
}

// Small pill (h 22) at x,y; returns its width so chips can be chained.
inline int16_t drawChip(CrowPanelDisplay &tft, int16_t x, int16_t y, const char *text, uint16_t color) {
  tft.setFont(&fonts::Font2);
  const int16_t w = tft.textWidth(text ? text : "") + 16;
  tft.fillRoundRect(x, y, w, 22, 11, kBg);
  tft.drawRoundRect(x, y, w, 22, 11, color);
  tft.setTextColor(color, kBg);
  tft.setTextDatum(lgfx::middle_left);
  tft.drawString(text ? text : "", x + 8, y + 11);
  restore(tft);
  return w;
}

inline void clearRow(CrowPanelDisplay &tft, uint8_t i) {
  Rect r = rowRect(i);
  tft.fillRect(r.x, r.y, r.w, r.h, kBg);
}

inline void drawFooter(CrowPanelDisplay &tft, uint16_t page, uint16_t pages) {
  tft.fillRect(0, 400, crowpanel::kScreenW, 56, kBg);
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
  tft.fillRect(0, kStatusY - 2, crowpanel::kScreenW, crowpanel::kScreenH - (kStatusY - 2), kBg);
  tft.setFont(&fonts::Font2);
  tft.setTextColor(kMuted, kBg);
  tft.setTextDatum(lgfx::top_left);
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
  tft.fillRect(0, top, crowpanel::kScreenW, crowpanel::kScreenH - top, kBg);
}

}  // namespace ui
}  // namespace crowpanel

#endif  // MARAUDER_CROWPANEL_ADV
