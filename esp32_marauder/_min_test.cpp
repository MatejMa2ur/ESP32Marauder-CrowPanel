// ponytail: the crowpanel_adv_5 env builds the full sketch; in from-source
// (ESP-IDF) mode build_src_filter is ignored, so exclude this standalone probe
// here to avoid duplicate setup()/loop(). Active only in the crowpanel_test env.
#ifndef MARAUDER_CROWPANEL_ADV
// Minimal faithful reproduction of Elecrow's CrowPanel-Advance-5 lesson-03
// bring-up: I2C scan, TCA9534 expander (0x18) backlight+reset, then LGFX RGB
// init + fillScreen(RED). Isolates panel/backlight hardware from Marauder.
#include <Arduino.h>
#include <Wire.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class _TestLGFX : public lgfx::LGFX_Device {
  lgfx::Bus_RGB _bus; lgfx::Panel_RGB _panel; lgfx::Touch_GT911 _touch;
 public:
  _TestLGFX() {
    { auto c=_panel.config(); c.memory_width=800; c.memory_height=480; c.panel_width=800; c.panel_height=480; c.offset_x=0; c.offset_y=0; _panel.config(c); }
    { auto c=_panel.config_detail(); c.use_psram=1; _panel.config_detail(c); }
    { auto c=_bus.config(); c.panel=&_panel;
      c.pin_d0=GPIO_NUM_21; c.pin_d1=GPIO_NUM_47; c.pin_d2=GPIO_NUM_48; c.pin_d3=GPIO_NUM_45; c.pin_d4=GPIO_NUM_38;
      c.pin_d5=GPIO_NUM_9; c.pin_d6=GPIO_NUM_10; c.pin_d7=GPIO_NUM_11; c.pin_d8=GPIO_NUM_12; c.pin_d9=GPIO_NUM_13; c.pin_d10=GPIO_NUM_14;
      c.pin_d11=GPIO_NUM_7; c.pin_d12=GPIO_NUM_17; c.pin_d13=GPIO_NUM_18; c.pin_d14=GPIO_NUM_3; c.pin_d15=GPIO_NUM_46;
      c.pin_henable=GPIO_NUM_42; c.pin_vsync=GPIO_NUM_41; c.pin_hsync=GPIO_NUM_40; c.pin_pclk=GPIO_NUM_39; c.freq_write=16000000;
      c.hsync_polarity=0; c.hsync_front_porch=8; c.hsync_pulse_width=4; c.hsync_back_porch=8;
      c.vsync_polarity=0; c.vsync_front_porch=8; c.vsync_pulse_width=4; c.vsync_back_porch=8; c.pclk_idle_high=1; _bus.config(c); }
    _panel.setBus(&_bus);
    { auto c=_touch.config(); c.x_min=0; c.x_max=800; c.y_min=0; c.y_max=480; c.pin_int=-1; c.bus_shared=false;
      c.offset_rotation=0; c.i2c_port=I2C_NUM_0; c.pin_sda=GPIO_NUM_15; c.pin_scl=GPIO_NUM_16; c.pin_rst=-1; c.freq=400000; c.i2c_addr=0x5D; _touch.config(c); _panel.setTouch(&_touch); }
    setPanel(&_panel);
  }
};
_TestLGFX tft;

static void ioex(uint8_t reg, uint8_t val, const char* tag) {
  Wire.beginTransmission(0x30); Wire.write(reg); Wire.write(val);
  uint8_t r = Wire.endTransmission();
  Serial.printf("  ioex write reg=0x%02X val=0x%02X -> endTransmission=%u (%s) [%s]\n",
                reg, val, r, r==0?"ACK":"NAK/err", tag);
}

// Single-byte command to the 0x30 backlight MCU. 0 = brightest, 245 = off.
// (Verbatim protocol from Elecrow CrowPanel-Advance-5 S3 V1.2/V1.3 lesson-03.)
static void sendI2CCommand(uint8_t cmd) {
  Wire.beginTransmission(0x30); Wire.write(cmd);
  Serial.printf("  cmd %u -> %u\n", cmd, Wire.endTransmission());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== MIN_TEST display bring-up (Elecrow V1.2/V1.3 flow) ===");
  pinMode(19, OUTPUT);
  Wire.begin(15, 16);
  delay(50);

  // Wait for the backlight MCU (0x30) + GT911 (0x5D). While absent, nudge with
  // cmd 250 (activate) and toggle GPIO1 (GT911 INT) to kick address select.
  for (int tries = 0; tries < 50; tries++) {
    Wire.beginTransmission(0x30); bool a30 = (Wire.endTransmission() == 0);
    Wire.beginTransmission(0x5D); bool a5d = (Wire.endTransmission() == 0);
    if (a30 && a5d) { Serial.println("MCU 0x30 + GT911 0x5D detected"); break; }
    Serial.printf("waiting... 0x30=%d 0x5D=%d\n", a30, a5d);
    sendI2CCommand(250);                 // activate
    pinMode(1, OUTPUT); digitalWrite(1, LOW); delay(120); pinMode(1, INPUT);
    delay(100);
  }

  sendI2CCommand(0);     // 0 = brightest, sent BEFORE init (factory order)

  tft.init();
  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(0x07E0);  // GREEN
  Serial.println(">>> panel up, backlight cmd 0, fill GREEN <<<");
}

void loop() {
  static uint32_t n = 0;
  uint16_t cols[] = {0x07E0, 0x001F, 0xF800, 0xFFFF};  // green, blue, red, white
  tft.fillScreen(cols[n % 4]);
  Serial.printf(">>> fill %u alive <<<\n", n++);
  delay(1500);
}

#endif  // !MARAUDER_CROWPANEL_ADV
