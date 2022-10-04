/*______Define LCD pins for AZ-touch _______*/
// Pins for the ILI9341 and ESP32dev
#define TFT_LED 15

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5  // Chip select control pin
#define TFT_DC    4  // Data Command control pin
#define TFT_RST   22  // Reset pin (could connect to RST pin)

#define TOUCH_CS 14
// #define TOUCH_IRQ 2
/*_______End of definitions______*/

/*______Assign pressure_______*/
#define MINPRESSURE 10
#define MAXPRESSURE 2000
/*_______Assigned______*/

/*____Calibrate TFT LCD_____*/
#define CALIBRATION_FILE "/TouchCalData1"
#define TS_MINX 370
#define TS_MINY 470
#define TS_MAXX 3700
#define TS_MAXY 3600
#define config_screen_width 320 
#define config_screen_height 240
/*______End of Calibration______*/

/*____Program specific constants_____*/
#define MAX_TEMPERATURE 24  
#define MIN_TEMPERATURE 10

#define LABEL0_FONT &FreeMono9pt7b // Key label font 1
#define LABEL1_FONT &FreeSansOblique9pt7b // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b    // Key label font 2
#define LABEL3_FONT &FreeSansBoldOblique24pt7b   // Key label font 2


enum { BOOT, COOLING, TEMP_OK, HEATING};        // Thermostat modes
/*______End of specific constants______*/


