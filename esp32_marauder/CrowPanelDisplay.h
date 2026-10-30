#pragma once

#include "configs.h"

#ifdef MARAUDER_CROWPANEL_ADV

// ---------------------------------------------------------------------------
// Display backend for the Elecrow CrowPanel 5.0" Advance (ESP32-S3).
//
// The panel is an 800x480 IPS driven by an ST7262 over the ESP32-S3 16-bit RGB
// LCD peripheral -- TFT_eSPI cannot drive it. Marauder's drawing code is written
// against the TFT_eSPI API, so (exactly like PoomDisplay.h does for the POOM
// board) we back a TFT_eSPI-compatible class with a different library and alias
// the names:  using TFT_eSPI = CrowPanelDisplay;
//
// We use LovyanGFX because (a) its API is intentionally TFT_eSPI-compatible
// (drawString/drawCentreString/setTextDatum/getTouch/setFreeFont all exist) and
// (b) Elecrow's own known-good driver for this exact board is a LovyanGFX
// config. The panel/bus/touch parameters below are copied verbatim from
// Elecrow's LovyanGFX_Driver.h (CrowPanel-Advance-5 repo, lesson-03).
// ---------------------------------------------------------------------------

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <Wire.h>

// Backlight on this V1.2/V1.3 board is a small MCU at I2C 0x30 (bus shared with
// GT911: SDA=15, SCL=16) that takes a SINGLE byte: 0 = brightest, 245 = off;
// cmd 250 = "activate". NOT register-addressed (0x18 TCA9534 was the old V1.0
// lesson -- this board's I2C scan is 0x30/0x51/0x5D, no expander). Must run
// before LGFX init() or the panel is dark. Verbatim from Elecrow's
// CrowPanel-Advance-5 S3 V1.2/V1.3 lesson-03 / factory flow.
#define CROWPANEL_ADV_BL_ADDR  0x30

// V1.1 controller (CROWPANEL_ADV_V11): different command set at the same 0x30 --
// 0x19 = activate touch, 0x10 = backlight enable, then a level 0x06..0x10
// (0x10 brightest). Sending the V1.2/V1.3 bytes to it leaves the screen black.
// Verified on-device in Marauder-GD (main/board.cpp). No HW revision detection.
#ifdef CROWPANEL_ADV_V11
  #define CROWPANEL_ADV_CMD_ACTIVATE 0x19
#else
  #define CROWPANEL_ADV_CMD_ACTIVATE 250
#endif

// Cod3rs theme: selected-row tint #1a1838, accent #755AFB.
#define CROWPANEL_SEL_BG  0x18C7
#define CROWPANEL_ACCENT  0x72DF

static inline void crowpanel_adv_bl_cmd(uint8_t cmd) {
  Wire.beginTransmission(CROWPANEL_ADV_BL_ADDR);
  Wire.write(cmd);
  Wire.endTransmission();
}

// Bring up the backlight MCU + GT911. Wait until both ACK; while absent, nudge
// with cmd 250 (activate) and toggle GPIO1 (GT911 INT) low to select addr 0x5D.
static inline void crowpanel_adv_panel_power_on() {
  Wire.begin(15, 16);
  delay(50);
  for (int tries = 0; tries < 50; tries++) {
    Wire.beginTransmission(0x30); bool a30 = (Wire.endTransmission() == 0);
    Wire.beginTransmission(0x5D); bool a5d = (Wire.endTransmission() == 0);
    if (a30 && a5d) break;
    crowpanel_adv_bl_cmd(CROWPANEL_ADV_CMD_ACTIVATE);
    pinMode(1, OUTPUT); digitalWrite(1, LOW); delay(120); pinMode(1, INPUT);
    delay(100);
  }
#ifdef CROWPANEL_ADV_V11
  crowpanel_adv_bl_cmd(0x10);                   // enable
  crowpanel_adv_bl_cmd(0x10);                   // level: brightest
#else
  crowpanel_adv_bl_cmd(0);                      // 0 = brightest, before init
#endif
}

class CrowPanelDisplay : public lgfx::LGFX_Device {
  lgfx::Bus_RGB   _bus;
  lgfx::Panel_RGB _panel;
  lgfx::Touch_GT911 _touch;

 public:
  CrowPanelDisplay() {
    {
      auto cfg = _panel.config();
      cfg.memory_width = 800;  cfg.memory_height = 480;
      cfg.panel_width  = 800;  cfg.panel_height  = 480;
      cfg.offset_x = 0;        cfg.offset_y = 0;
      _panel.config(cfg);
    }
    {
      auto cfg = _panel.config_detail();
      cfg.use_psram = 1;  // 800*480*2 = 768 KB framebuffer lives in PSRAM
      _panel.config_detail(cfg);
    }
    {
      auto cfg = _bus.config();
      cfg.panel = &_panel;
      // d0..d4 = B0..B4, d5..d10 = G0..G5, d11..d15 = R0..R4
      cfg.pin_d0  = GPIO_NUM_21; cfg.pin_d1  = GPIO_NUM_47; cfg.pin_d2  = GPIO_NUM_48;
      cfg.pin_d3  = GPIO_NUM_45; cfg.pin_d4  = GPIO_NUM_38;
      cfg.pin_d5  = GPIO_NUM_9;  cfg.pin_d6  = GPIO_NUM_10; cfg.pin_d7  = GPIO_NUM_11;
      cfg.pin_d8  = GPIO_NUM_12; cfg.pin_d9  = GPIO_NUM_13; cfg.pin_d10 = GPIO_NUM_14;
      cfg.pin_d11 = GPIO_NUM_7;  cfg.pin_d12 = GPIO_NUM_17; cfg.pin_d13 = GPIO_NUM_18;
      cfg.pin_d14 = GPIO_NUM_3;  cfg.pin_d15 = GPIO_NUM_46;
      cfg.pin_henable = GPIO_NUM_42;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_40;
      cfg.pin_pclk    = GPIO_NUM_39;
      // LovyanGFX streams the framebuffer straight from PSRAM (no bounce buffer,
      // unlike esp_lcd), so PSRAM contention from WiFi/flash underruns the DMA
      // and the image shears. Lower PCLK = more headroom. Tune with the flag.
#ifndef CROWPANEL_ADV_PCLK_HZ
  #define CROWPANEL_ADV_PCLK_HZ 12000000
#endif
      cfg.freq_write  = CROWPANEL_ADV_PCLK_HZ;
      cfg.hsync_polarity = 0; cfg.hsync_front_porch = 8; cfg.hsync_pulse_width = 4; cfg.hsync_back_porch = 8;
      cfg.vsync_polarity = 0; cfg.vsync_front_porch = 8; cfg.vsync_pulse_width = 4; cfg.vsync_back_porch = 8;
      cfg.pclk_idle_high = 1;
      _bus.config(cfg);
    }
    _panel.setBus(&_bus);
    {
      auto cfg = _touch.config();
      cfg.x_min = 0; cfg.x_max = 800;
      cfg.y_min = 0; cfg.y_max = 480;
      cfg.pin_int = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = I2C_NUM_0;
      cfg.pin_sda = GPIO_NUM_15;
      cfg.pin_scl = GPIO_NUM_16;
      cfg.pin_rst = -1;
      cfg.freq = 400000;
      cfg.i2c_addr = 0x5D;  // alt 0x14 depending on INT level at boot
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }

  // ---- TFT_eSPI compatibility shims (names the rest of Marauder calls) ----

  // Marauder calls tft.init(). The backlight + reset live on the TCA9534 I2C
  // expander, which must be driven BEFORE the RGB panel init -- do that first.
  bool init() {
    crowpanel_adv_panel_power_on();
    return lgfx::LGFX_Device::init();
  }

  // RGB panel renders directly from the PSRAM framebuffer, so there is no
  // off-screen buffer to flush. Present only so tft.display() compiles.
  void display(bool = false) {}

  // GT911 reports absolute panel coordinates; there is nothing to calibrate,
  // so the TFT_eSPI calibration setter is a no-op here.
  void setTouch(uint16_t * = nullptr) {}

  // TFT_eSPI's getTouch has a trailing threshold arg; LovyanGFX's does not.
  bool getTouch(uint16_t *x, uint16_t *y, uint16_t = 600) {
    return lgfx::LGFX_Device::getTouch(x, y);
  }

  // Marauder calls drawXBitmap/setTextColor with mixed int/uint16_t color
  // arguments. LovyanGFX's templated versions can't deduce a single color type
  // from those, so give them concrete uint16_t overloads. `using` keeps the
  // inherited single-color / shorter overloads visible (a derived declaration
  // otherwise hides all base overloads of the same name).
  using lgfx::LGFX_Device::drawXBitmap;
  using lgfx::LGFX_Device::setTextColor;
  void drawXBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w,
                   int32_t h, uint16_t fg, uint16_t bg) {
    if (fg != TFT_BLACK) { lgfx::LGFX_Device::drawXBitmap(x, y, bitmap, w, h, fg, bg); return; }
    // ponytail: menu icons are "black ink on a coloured box"; keep the black
    // transparent so the selected-row tint shows through. Per-pixel, icons are 22x22.
    const int32_t bw = (w + 7) / 8;
    startWrite();
    for (int32_t j = 0; j < h; j++)
      for (int32_t i = 0; i < w; i++)
        if (!(pgm_read_byte(bitmap + j * bw + i / 8) & (1 << (i & 7))))
          drawPixel(x + i, y + j, bg);
    endWrite();
  }
  void setTextColor(uint16_t fg, uint16_t bg, bool /*bg_fill*/) {
    lgfx::LGFX_Device::setTextColor(fg, bg);
  }
};

// Generic button, copied from PoomButton (pure geometry over the gfx pointer;
// every primitive it calls exists on LovyanGFX). Matches the TFT_eSPI_Button API
// surface Marauder uses: initButton / setLabelDatum / drawButton / contains /
// press / isPressed / justReleased.
class CrowPanelButton {
 public:
  void initButton(CrowPanelDisplay *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                  uint16_t outline, uint16_t fill, uint16_t textcolor,
                  char *label, uint8_t textsize) {
    gfx_ = gfx; x_ = x; y_ = y; w_ = w; h_ = h; outline_ = outline; fill_ = fill;
    text_ = textcolor; text_size_ = textsize; label_ = label ? label : "";
  }
  void setLabelDatum(int16_t x, int16_t y, uint8_t = MC_DATUM) { label_dx_ = x; label_dy_ = y; }
  void drawButton(bool inverted = false, String long_name = "") {
    if (!gfx_) return;
    // Selected row: dark accent tint + outline instead of the TFT_eSPI-style
    // inverted fill (an 800x48 bar in the item colour -- usually green).
    const uint16_t fill = inverted ? CROWPANEL_SEL_BG : fill_;
    const uint16_t text = text_;
    gfx_->fillRect(x_ - w_ / 2, y_ - h_ / 2, w_, h_, fill);
    gfx_->drawRect(x_ - w_ / 2, y_ - h_ / 2, w_, h_, inverted ? CROWPANEL_ACCENT : outline_);
    gfx_->setTextColor(text, fill);
    gfx_->setTextSize(text_size_);
    // Middle datum so the label stays vertically centred at any KEY_H/textsize
    // (the TFT_eSPI-style top datum + fixed -4 only fits 22px keys).
    gfx_->setTextDatum(lgfx::middle_center);
    gfx_->drawString(long_name.length() ? long_name : label_, x_ + label_dx_, y_ + label_dy_);
    gfx_->setTextDatum(lgfx::top_left);
  }
  bool contains(int16_t x, int16_t y) const {
    return x >= x_ - w_ / 2 && x < x_ + w_ / 2 && y >= y_ - h_ / 2 && y < y_ + h_ / 2;
  }
  void press(bool pressed) { laststate_ = currstate_; currstate_ = pressed; }
  bool isPressed() const { return currstate_; }
  bool justReleased() const { return !currstate_ && laststate_; }

 private:
  CrowPanelDisplay *gfx_ = nullptr;
  int16_t x_ = 0, y_ = 0, w_ = 0, h_ = 0;
  int16_t label_dx_ = 0, label_dy_ = 0;
  uint16_t outline_ = TFT_WHITE, fill_ = TFT_BLACK, text_ = TFT_WHITE;
  uint8_t text_size_ = 1;
  String label_;
  bool currstate_ = false, laststate_ = false;
};

using TFT_eSPI = CrowPanelDisplay;
using TFT_eSPI_Button = CrowPanelButton;

#endif  // MARAUDER_CROWPANEL_ADV
