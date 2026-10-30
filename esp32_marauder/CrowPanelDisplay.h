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
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
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
// with the activate cmd and toggle GPIO1 (GT911 INT) low to select addr 0x5D.
// V1.1: the 0x30 MCU keeps state across ESP warm resets (and shows its self-test
// colour cycle if not re-activated), so activate + INT pulse ALWAYS run once first.
static inline void crowpanel_adv_panel_power_on() {
  Wire.begin(15, 16);
  Wire.setClock(400000);
  delay(50);
#ifdef CROWPANEL_ADV_V11
  crowpanel_adv_bl_cmd(CROWPANEL_ADV_CMD_ACTIVATE);
  pinMode(1, OUTPUT); digitalWrite(1, LOW); delay(120); pinMode(1, INPUT);
  delay(100);
#endif
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

// ponytail: LovyanGFX's stock Bus_RGB streams the RGB DMA straight from the
// PSRAM framebuffer with NO bounce buffer, so PSRAM contention from WiFi/flash
// starves the pixel stream -> rolling colour bars (or a white screen at low
// PCLK). Hand scanout to the native esp_lcd RGB driver with a 10-line internal-
// SRAM bounce buffer -- the DMA then reads from SRAM and an ISR refills it from
// PSRAM, so contention can't starve it. Config mirrors the known-good
// Marauder-GD main/board.cpp. LovyanGFX keeps doing ALL the drawing; it finds
// the framebuffer through the bus's virtual getDMABuffer(), which we point at
// esp_lcd's framebuffer -- so no Marauder drawing code changes.
class CrowPanelBus : public lgfx::Bus_RGB {
  esp_lcd_panel_handle_t _panel_h = nullptr;
  void* _fb = nullptr;
 public:
  bool init(void) override {
    if (_panel_h) return _fb != nullptr;   // one-shot, like the LGFX init guard
    const auto& c = config();
    esp_lcd_rgb_panel_config_t cfg = {};
    cfg.clk_src               = LCD_CLK_SRC_DEFAULT;
    cfg.data_width            = 16;
    cfg.bits_per_pixel        = 16;
    cfg.num_fbs               = 1;
    cfg.bounce_buffer_size_px = 800 * 10;   // two 16 KB internal-SRAM buffers
    cfg.dma_burst_size        = 64;
    cfg.hsync_gpio_num        = c.pin_hsync;
    cfg.vsync_gpio_num        = c.pin_vsync;
    cfg.de_gpio_num           = c.pin_henable;
    cfg.pclk_gpio_num         = c.pin_pclk;
    cfg.disp_gpio_num         = -1;
    for (int i = 0; i < 16; ++i) cfg.data_gpio_nums[i] = c.pin_data[i];
    cfg.timings.pclk_hz           = c.freq_write;
    cfg.timings.h_res             = 800;
    cfg.timings.v_res             = 480;
    cfg.timings.hsync_pulse_width = c.hsync_pulse_width;
    cfg.timings.hsync_back_porch  = c.hsync_back_porch;
    cfg.timings.hsync_front_porch = c.hsync_front_porch;
    cfg.timings.vsync_pulse_width = c.vsync_pulse_width;
    cfg.timings.vsync_back_porch  = c.vsync_back_porch;
    cfg.timings.vsync_front_porch = c.vsync_front_porch;
    cfg.timings.flags.pclk_active_neg = c.pclk_active_neg;  // 1, as Marauder-GD
    cfg.flags.fb_in_psram = 1;
    if (esp_lcd_new_rgb_panel(&cfg, &_panel_h) != ESP_OK) return false;
    esp_lcd_panel_reset(_panel_h);
    esp_lcd_panel_init(_panel_h);           // starts the bounce-buffered scanout
    if (esp_lcd_rgb_panel_get_frame_buffer(_panel_h, 1, &_fb) != ESP_OK) _fb = nullptr;
    return _fb != nullptr;
  }
  uint8_t* getDMABuffer(uint32_t) override { return (uint8_t*)_fb; }
  void release(void) override {
    if (_panel_h) { esp_lcd_panel_del(_panel_h); _panel_h = nullptr; _fb = nullptr; }
  }
};

// percent 10..100. V1.1: enable (0x10) then one of 5 levels; V1.2/1.3: single
// byte, 0 = brightest. Mirrors board_brightness() in Marauder-GD.
static inline void crowpanel_adv_set_brightness(uint8_t percent) {
  if (percent < 10) percent = 10;
  if (percent > 100) percent = 100;
#ifdef CROWPANEL_ADV_V11
  static const uint8_t levels[] = {0x06, 0x07, 0x08, 0x09, 0x10};
  uint8_t i = (percent - 1) / 20;
  crowpanel_adv_bl_cmd(0x10);
  crowpanel_adv_bl_cmd(levels[i > 4 ? 4 : i]);
#else
  crowpanel_adv_bl_cmd(244 - percent * 244 / 100);
#endif
}

class CrowPanelDisplay : public lgfx::LGFX_Device {
  CrowPanelBus    _bus;
  lgfx::Panel_RGB _panel;

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
      // Pixel clock. CrowPanelBus hands scanout to esp_lcd with a bounce buffer,
      // so 16 MHz (Marauder-GD's proven value) is stable; the old bufferless
      // path needed <=12 MHz and still sheared. Override the flag to retune.
#ifndef CROWPANEL_ADV_PCLK_HZ
  #define CROWPANEL_ADV_PCLK_HZ 16000000
#endif
      cfg.freq_write  = CROWPANEL_ADV_PCLK_HZ;
      cfg.hsync_polarity = 0; cfg.hsync_front_porch = 8; cfg.hsync_pulse_width = 4; cfg.hsync_back_porch = 8;
      cfg.vsync_polarity = 0; cfg.vsync_front_porch = 8; cfg.vsync_pulse_width = 4; cfg.vsync_back_porch = 8;
      _bus.config(cfg);
    }
    _panel.setBus(&_bus);
    setPanel(&_panel);
  }

  // ---- TFT_eSPI compatibility shims (names the rest of Marauder calls) ----

  // Marauder calls tft.init(). The backlight MCU (I2C 0x30) must be activated
  // BEFORE the RGB panel init, so run panel power-on first.
  bool init() {
    // Marauder calls tft.init() on every scan/menu switch. Re-running it would
    // repeat panel power-on (backlight reset, blank) and LGFX init on a live
    // panel, so bring up once; later calls are no-ops.
    static bool ready = false;
    if (ready) return true;
    crowpanel_adv_panel_power_on();
    ready = lgfx::LGFX_Device::init();
    return ready;
  }

  // RGB panel renders directly from the PSRAM framebuffer, so there is no
  // off-screen buffer to flush. Present only so tft.display() compiles.
  void display(bool = false) {}

  // GT911 reports absolute panel coordinates; there is nothing to calibrate,
  // so the TFT_eSPI calibration setter is a no-op here.
  void setTouch(uint16_t * = nullptr) {}

  // Touch is read only by gt911.h (Display::updateTouch); LGFX has no touch
  // driver here. Kept so TFT_eSPI-style callers still compile.
  bool getTouch(uint16_t *, uint16_t *, uint16_t = 600) { return false; }

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
