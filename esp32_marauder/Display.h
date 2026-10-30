#pragma once

#ifndef Display_h
#define Display_h

#include "configs.h"

#ifdef HAS_SCREEN

#include <FS.h>
#include <functional>
#include <LinkedList.h>
#include <SPI.h>
#include "SPIFFS.h"
#include "Assets.h"
#include "BootSplash.h"
#include "MemoryGuard.h"

#ifdef MARAUDER_POOM
  #include "PoomDisplay.h"
#elif defined(MARAUDER_CROWPANEL_ADV)
  #include "CrowPanelDisplay.h"
  #include "CrowPanelLayout.h"
#else
  #include <TFT_eSPI.h>
#endif

// Reject board/display configuration mismatches at compile time. A mismatched
// TFT setup can boot normally while driving the wrong controller and pins.
#if defined(MARAUDER_CYD_3_5_INCH) && !defined(ST7796_DRIVER)
  #error "MARAUDER_CYD_3_5_INCH requires User_Setup_cyd_3_5_inch.h (ST7796_DRIVER)"
#endif
#if (defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)) && !defined(ST7789_2_DRIVER)
  #error "Cardputer targets require their ST7789_2_DRIVER TFT_eSPI setup"
#endif
#if defined(MARAUDER_M5STACK_CORE2_AWS) && !defined(ILI9341_DRIVER)
  #error "M5Stack Core2 target requires its ILI9341_DRIVER TFT_eSPI setup"
#endif

#ifdef HAS_CYD_TOUCH
  #include <XPT2046_Touchscreen.h>
#endif

#ifdef HAS_CAP_TOUCH
  #ifdef MARAUDER_CROWPANEL_ADV
    #include "gt911.h"
  #else
    #include "ft6336.h"
  #endif
#endif

// WiFi stuff
#define OTA_UPDATE 100
#define SHOW_INFO 101
#define WIFI_SCAN_OFF 0
#define WIFI_SCAN_PROBE 1
#define WIFI_SCAN_AP 2
#define WIFI_SCAN_PWN 3
#define WIFI_SCAN_EAPOL 4
#define WIFI_SCAN_DEAUTH 5
#define WIFI_SCAN_ALL 6
#define WIFI_PACKET_MONITOR 7
#define WIFI_ATTACK_BEACON_SPAM 8
#define WIFI_ATTACK_RICK_ROLL 9
#define BT_SCAN_ALL 10
#define BT_SCAN_SKIMMERS 11
#define WIFI_SCAN_ESPRESSIF 12
#define LV_JOIN_WIFI 13
#define LV_ADD_SSID 14
#define WIFI_ATTACK_BEACON_LIST 15

#define RED_KEY ";red;"
#define GREEN_KEY ";grn;"
#define CYAN_KEY ";cyn;"
#define MAGENTA_KEY ";mgn;"
#define WHITE_KEY ";wht;"

#define UP_BUTTON     0
#define SELECT_BUTTON 1
#define DOWN_BUTTON   2

#define EXIT_BUTTON_INDEX 0 // 6
#define CHAN_MINUS_INDEX  1 // 4
#define CHAN_PLUS_INDEX   2 // 5
#define CHAN_HOP_INDEX    5 // 7
#define X_MINUS_INDEX     6
#define X_PLUS_INDEX      7
#define Y_MINUS_INDEX     8
#define Y_PLUS_INDEX      9

class Display
{
  private:
    bool SwitchOn = false;
    
    bool run_setup = true;
    
    // For the byte we read from the serial port
    byte data = 0;
    
    // A few test variables used during debugging
    boolean change_colour = 1;
    boolean selected = 1;

    #ifdef MARAUDER_M5STACK_CORE2_AWS
      bool core2_full_touch_input = false;
      int8_t core2_front_button = -1;
      uint32_t core2_front_button_seen = 0;
    #endif

    void drawFrame();

    #ifdef SCREEN_BUFFER
      void scrollScreenBuffer(bool down = false);
    #endif
    void processAndPrintString(TFT_eSPI& tft, const String& originalString);

  public:
    Display();
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSPI_Button key[BUTTON_ARRAY_LEN + 4];
    const String PROGMEM version_number = MARAUDER_VERSION;

    #ifdef HAS_CYD_TOUCH
      SPIClass touchscreenSPI;
      XPT2046_Touchscreen touchscreen;
    #endif

    bool printing = false;
    bool loading = false;
    bool tteBar = false;
    bool draw_tft = false;
    bool exit_draw = false;
    bool headless_mode = false;
    // Boot loading-bar geometry, set by drawBootSplash; boot_bar_w_ == 0 means
    // no bar (drawBootProgress is then a no-op).
    int16_t boot_bar_x_ = 0, boot_bar_y_ = 0, boot_bar_w_ = 0, boot_bar_h_ = 0;
    marauder::QueueBackpressureState display_queue_pressure{
        DISPLAY_BUFFER_LIMIT};

    uint8_t TOP_FIXED_AREA_2 = 48;
    uint8_t print_delay_1, print_delay_2 = 10;
    uint16_t current_banner_pos = SCREEN_WIDTH;

    LinkedList<String>* display_buffer;

    #ifdef SCREEN_BUFFER
      LinkedList<String>* screen_buffer;
    #endif

    // The initial y coordinate of the top of the bottom text line
    uint16_t yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;

    // Keep track of the drawing x coordinate
    uint16_t xPos = 0;

    // The initial y coordinate of the top of the scrolling area
    uint16_t yStart = TOP_FIXED_AREA_2;
    // yArea must be a integral multiple of TEXT_HEIGHT
    uint16_t yArea = YMAX - TOP_FIXED_AREA_2 - BOT_FIXED_AREA;

    // We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
    // for a full width line, meanwhile the serial buffer may be filling... and overflowing
    // We can speed up scrolling of short text lines by just blanking the character we drew
    int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

    int8_t menuButton(uint16_t *x, uint16_t *y, bool pressed, bool check_hold = false);
    uint8_t updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold = 600);
    bool isTouchHeld(uint16_t threshold = 600);
    #ifdef MARAUDER_M5STACK_CORE2_AWS
      void setFullTouchInput(bool enabled) { core2_full_touch_input = enabled; }
      bool isFrontButtonTouch() const { return core2_front_button >= 0; }
    #endif
    void tftDrawGraphObjects(byte x_scale);
    void tftDrawEapolColorKey(bool filter = false);
    void tftDrawColorKey();
    void tftDrawXScaleButtons(byte x_scale);
    void tftDrawYScaleButtons(byte y_scale);
    void tftDrawChannelScaleButtons(int set_channel, bool lnd_an = true);
    void tftDrawExitScaleButtons(bool lnd_an = true);
    void tftDrawChanHopButton(bool lnd_an = true, bool en = false);
    void buildBanner(String msg, int xpos);
    void clearScreen();
    bool queueLine(const String& line);
    bool queueCriticalLine(const String& line);
    void displayBuffer(bool do_clear = false);
    void drawBootSplash();
    // Boot loading bar. No-op unless drawBootSplash set up a bar (CrowPanel);
    // safe to call unconditionally from the boot sequence. pct is 0..100.
    void drawBootProgress(uint8_t pct);
    void getTouchWhileFunction(bool pressed);
    void init();
    void RunSetup();
    void showCenterText(const char* text, int y, bool small_pp = false, uint8_t text_size = BANNER_TEXT_SIZE);
    void touchToExit();
    void scanTitleBand(uint16_t color);
    void scanTitle(const String& title);
    #ifdef MARAUDER_CROWPANEL_ADV
      // GD-style scan header (title, STOP) + cleared body; toolbar state below.
      char crowpanel_title[48] = "";
      bool crowpanel_toolbar = false;  // channel/hop toolbar shown under the header
      bool crowpanel_hop = false;
      uint8_t crowpanel_ch = 1;
      bool crowpanel_hop_en = true;    // false: HOP drawn disabled and ignored (packet monitor)
      void crowpanelScanHeader(const char* title);
      void crowpanelToolbar();
      // One 16px text row in the body at x=18 (text size 2), cleared first.
      void crowpanelBodyLine(const char* text, int16_t y, uint16_t color);
      // Marauder-GD stat card row (statRect/kStatRowBottom from CrowPanelLayout.h).
      // crowpanelStatsRow draws n (<=4) cards and, if bodyTop > 0, sets
      // TOP_FIXED_AREA_2 so body text/rows start below them. crowpanelStatsUpdate
      // redraws only a card's value area; gate calls with crowpanelStatsDue() to
      // cap refresh at ~2Hz. crowpanelStatsClear resets state when a screen with
      // no stats is drawn.
      static constexpr uint8_t kMaxCrowStats = 4;
      crowpanel::Rect crow_stat_rect[kMaxCrowStats];
      uint16_t crow_stat_color[kMaxCrowStats] = {0, 0, 0, 0};
      uint8_t crow_stat_n = 0;
      uint32_t crow_stats_last_ms = 0;
      void crowpanelStatsRow(const crowpanel::Rect* rects, uint8_t n,
                             const char* const* labels, const char* const* values,
                             const uint16_t* colors, int16_t bodyTop);
      void crowpanelStatsUpdate(uint8_t i, const char* value);
      bool crowpanelStatsDue(uint32_t now);
      void crowpanelStatsClear();
    #endif
    void twoPartDisplay(String center_text);
    void updateBanner(String msg);
    void setCalData(bool landscape = false);
};
#endif
#endif
