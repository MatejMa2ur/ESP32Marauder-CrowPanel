#pragma once
#ifndef gt911_h
#define gt911_h

#include "configs.h"

#if defined(HAS_CAP_TOUCH) && defined(MARAUDER_CROWPANEL_ADV)

#include <Wire.h>

// Raw GT911 capacitive-touch reader for the CrowPanel 5.0" Advance, mirroring
// the ft6336.h contract so Display.cpp can use it the same way:
//   gt911_init()                    -- bring up the I2C bus / controller
//   gt911_read_raw(&x, &y) -> int8  -- 1 active touch, 0 confirmed release,
//                                      -1 transport error (don't treat as tap)
// Coordinates are the panel's native landscape space (0..799 x 0..479); the
// rotation transform lives in Display::updateTouch.
//
// GT911 uses 16-bit register addresses. Default I2C address is 0x5D when INT/RST
// are left floating at boot (they are -1 here). Status reg 0x814E: bit7 = buffer
// ready, low nibble = point count; point 1 coords start at 0x8150; the buffer
// flag MUST be cleared by writing 0 back to 0x814E after each read.

#define GT911_ADDR        0x5D
#define GT911_REG_STATUS  0x814E
#define GT911_REG_POINT1  0x8150

static bool _gt911_read(uint16_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    if (Wire.endTransmission(false) != 0) return false;
    uint8_t got = Wire.requestFrom((int)GT911_ADDR, (int)len);
    if (got != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static void _gt911_write_reg(uint16_t reg, uint8_t value) {
    Wire.beginTransmission(GT911_ADDR);
    Wire.write((uint8_t)(reg >> 8));
    Wire.write((uint8_t)(reg & 0xFF));
    Wire.write(value);
    Wire.endTransmission();
}

static void gt911_init() {
    Wire.begin(CTP_SDA, CTP_SCL, 400000U);
    delay(10);

    uint8_t product_id[4] = {0};
    bool ok = _gt911_read(0x8140, product_id, 4);  // "911\0"
    Serial.printf("[Touch] GT911 ID: %c%c%c%s\n",
                  product_id[0], product_id[1], product_id[2],
                  ok ? "" : " (read failed)");
}

static int8_t gt911_read_raw(uint16_t *raw_x, uint16_t *raw_y) {
    uint8_t status = 0;
    if (!_gt911_read(GT911_REG_STATUS, &status, 1)) return -1;

    // Buffer not ready yet: no new data, but not a confirmed release either.
    if (!(status & 0x80)) return 0;

    const uint8_t points = status & 0x0F;
    int8_t result = 0;  // buffer ready with 0 points == real release
    if (points >= 1) {
        uint8_t p[4];
        if (_gt911_read(GT911_REG_POINT1, p, 4)) {
            *raw_x = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
            *raw_y = (uint16_t)p[2] | ((uint16_t)p[3] << 8);
            result = 1;
        } else {
            result = -1;
        }
    }

    // Always clear the buffer-ready flag so the next frame can be reported.
    _gt911_write_reg(GT911_REG_STATUS, 0);
    return result;
}

#endif  // HAS_CAP_TOUCH && MARAUDER_CROWPANEL_ADV
#endif  // gt911_h
