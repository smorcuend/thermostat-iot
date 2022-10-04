#define ILI9341_ULTRA_DARKGREY 0x632C
#define BACKGROUND      TFT_WHITE
#define TFT_GREY 0xBDF7

// Configure screen colors
#ifdef ESP32
  uint16_t palette[] = {
    0x0000,   // 0: Black
    0xFFFF,   // 1: White
    0x001F,   // 2: Blue
    0xFFE0,   // 3: Yellow
    0x07E0,   // 4: Green
    0xF800,   // 5: Red
    0xFE19,   // 6: Pink
    0x07FF    // 7: Cyan
  };
#endif