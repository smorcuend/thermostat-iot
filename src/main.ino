#include <Arduino.h>
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>

#include <TimeOut.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "usergraphics.h"
#include "settings.h"
#include "secrets.h"

// #define _debug

#define I2C_SDA 25
#define I2C_SCL 32

#define RELAY_PIN 33 

#define SEALEVELPRESSURE_HPA (1013.25)

WiFiUDP ntpUDP;
// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
int GTMOffset = 2;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", GTMOffset*60*60, 60000);

TwoWire I2CBME = TwoWire(0);
Adafruit_BME280 bme;

TFT_eSPI tft = TFT_eSPI();

uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
uint8_t Thermostat_mode = BOOT;
float iRoom_temperature = 0.0;
float iSet_temperature = 30.0;

bool Touch_pressed = false;

Interval interval0;
void intervalBlackOut();
void readBME280();
void updateTime();

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  #ifdef _debug
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  #endif
}

// Define display helpers
void displayInit(int width, int height) {
  tft.init();
  tft.setRotation(1); //Landscape

  #ifdef _debug
  Serial.print("tftx=");
  Serial.print(tft.width());
  Serial.print(" tfty=");
  Serial.println(tft.height());
  #endif

  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);
}

void initTouchScreen() {
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);    // LOW to Turn on with assembled T1 and R2
  delay(1);

  displayInit(config_screen_width, config_screen_height);
}

void touch_calibrate() {
  uint16_t calibrationData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char*)calibrationData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }
  if (calDataOK) {
    // calibration data valid
    tft.setTouch(calibrationData);
  }
  else {
    // data not valid. recalibrate
    tft.calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char*)calibrationData, 14);
      f.close();
    }
  }

  tft.fillScreen((0xFFFF));
}


/********************************************************************
 * @brief     update of the value for room temperature on the screen
 *            (in the small grey circle)
 * @param[in] None
 * @return    None
 *********************************************************************/
void update_Room_temp() {
  int16_t x1, y1;
  uint16_t w, h;
  String curValue = String(static_cast<int>(iRoom_temperature));
  int str_len = curValue.length() + 1;
  char char_array[str_len];
  curValue.toCharArray(char_array, str_len);
  tft.fillRect(36, 200, 30, 21, ILI9341_ULTRA_DARKGREY);
  tft.setTextColor(ILI9341_WHITE, ILI9341_ULTRA_DARKGREY);
  tft.setFreeFont(LABEL2_FONT);
  // tft.getTextBounds(char_array, 40, 220, &x1, &y1, &w, &h);
  tft.setCursor(38 - w, 220);
  tft.print(char_array);
}

/********************************************************************
 * @brief     update of the value for set temperature on the screen
 *            (in the big colored circle)
 * @param[in] None
 * @return    None
 *********************************************************************/
void update_SET_temp() {
  int16_t x1, y1;
  uint16_t w, h;
  String curValue = String(static_cast<int>(iSet_temperature));
  int str_len = curValue.length() + 1;
  char char_array[str_len];
  curValue.toCharArray(char_array, str_len);
  tft.fillRect(70, 96, 60, 50, ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFreeFont(LABEL3_FONT);
  // tft.getTextDatum(char_array, 80, 130, &x1, &y1, &w, &h);
  tft.setCursor(70 - w, 130);
  tft.print(char_array);
}


/********************************************************************
 * @brief     drawing of the circles in main screen including the value
 *            of room temperature
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_circles(int room_temp, int set_temp) {

  //draw big circle 
  unsigned char i;
  if (room_temp < set_temp) {
    // heating - red
    for (i = 0; i < 10; i++) tft.drawCircle(120, 120, 80 + i, ILI9341_RED);
  }
  else if (room_temp > set_temp) {
    // cooling - blue
    for (i = 0; i < 10; i++) tft.drawCircle(120, 120, 80 + i, ILI9341_BLUE);
  }
  else {
    // Temperature ok
    for (i = 0; i < 10; i++) tft.drawCircle(120, 120, 80 + i, ILI9341_GREEN);
  }

  //draw small 
  tft.fillCircle(60, 200, 40, ILI9341_ULTRA_DARKGREY);

  //draw °C in big circle
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFreeFont(LABEL2_FONT);
  tft.setCursor(135, 100);
  tft.print("o");
  tft.setFreeFont(LABEL3_FONT);
  tft.setCursor(140, 130);
  tft.print("C");

  // draw room and °C in small circle
  tft.setTextColor(ILI9341_WHITE, ILI9341_ULTRA_DARKGREY);
  tft.setFreeFont(LABEL1_FONT);
  tft.setCursor(75, 220);
  tft.print("C");
  // Draw º char
  tft.drawCircle(74, 204, 2, ILI9341_WHITE);
  tft.drawCircle(74, 204, 3, ILI9341_WHITE);
  tft.setFreeFont(LABEL1_FONT);
  tft.setCursor(35, 190);
  tft.print("Room");
  update_Room_temp();
}


/********************************************************************
 * @brief     update of the color of the big circle according the
 *            difference between set and room temperature
 * @param[in] None
 * @return    None
 *********************************************************************/
void update_circle_color() {
  int iRoom_int = iRoom_temperature / 1.0;
  int iSet_int = iSet_temperature / 1.0;
  // HEATING 
  if ((iRoom_int < iSet_int) && (Thermostat_mode != HEATING)) {
    Thermostat_mode = HEATING;
    draw_circles(iRoom_int, iSet_int);
  }

  // COOLING 
  if ((iRoom_int > iSet_int) && (Thermostat_mode != COOLING)) {
    Thermostat_mode = COOLING;
    draw_circles(iRoom_int, iSet_int);
  }

  // Temperature ok 
  if ((iRoom_int == iSet_int) && (Thermostat_mode != TEMP_OK)) {
    Thermostat_mode = TEMP_OK;
    draw_circles(iRoom_int, iSet_int);
  }
}

/********************************************************************
 * @brief     drawing of the both buttons for setting temperature up
 *            and down
 * @param[in] None
 * @return    None
 *********************************************************************/
void draw_up_down_button() {
  //up button 
  tft.fillTriangle(265, 30, 280, 50, 250, 50, ILI9341_WHITE);
  tft.drawCircle(265, 45, 20, ILI9341_WHITE);

  //down button
  tft.fillTriangle(265, 210, 280, 190, 250, 190, ILI9341_WHITE);
  tft.drawCircle(265, 195, 20, ILI9341_WHITE);
}

/********************************************************************
 * @brief     detecting pressed buttons with the given touchscreen values
 * @param[in] None
 * @return    None
 *********************************************************************/
void DetectButtons(uint16_t X, uint16_t Y) {
  // button UP
  if ((X > 250) && (Y > 200)) {
    if (iSet_temperature < MAX_TEMPERATURE) iSet_temperature++;
    tone(21, 2000, 10);
    // holdingRegs[SET_TEMP] = iSet_temperature;
    update_SET_temp();
    update_circle_color();
  }

  // button DWN
  if ((X < 70) && (Y > 200)) {
    if (iSet_temperature > MIN_TEMPERATURE) iSet_temperature--;
    tone(21, 2000, 10);
    // holdingRegs[SET_TEMP] = iSet_temperature;
    update_SET_temp();
    update_circle_color();
  }
}



/********************************************************************
 * @brief     shows the intro screen in setup procedure
 * @param[in] None
 * @return    None
 *********************************************************************/
void IntroScreen() {
  // draw circles
  update_circle_color();

  // draw temperature up/dwn buttons
  draw_up_down_button();

  update_SET_temp();
  update_Room_temp();
}

/********************************************************************
 * @brief     manage touch events
 * @param[in] None
 * @return    None
 *********************************************************************/
void touchscreen() {
  uint16_t x, y;

  if (tft.getTouch(&x, &y, 1000)) {
    if (Touch_pressed == false) {
      Touch_pressed = true;
      digitalWrite(TFT_LED, LOW);
    }
    DetectButtons(x, y);

    // Testing purpose
    #ifdef _debug
    tft.fillRect(0, 0, 80, 50, TFT_BLACK);
    tft.setFreeFont(LABEL0_FONT);
    tft.setTextSize(1);
    tft.setCursor(5, 15, 2);
    tft.printf("x: %i ", x);
    tft.setCursor(5, 35, 2);
    tft.printf("y: %i ", y);
    #endif
  }
}

/********************************************************************
 * @brief     manage display blackout
 * @param[in] None
 * @return    None
 *********************************************************************/
void intervalBlackOut() {
  Touch_pressed = false;
  digitalWrite(TFT_LED, HIGH);
}

void updateTime() {
  String time = timeClient.getFormattedTime();
  tft.fillRect(0, 0, 100, 30, TFT_BLACK);
  tft.setFreeFont(LABEL0_FONT);
  tft.setTextSize(1);
  tft.setCursor(5, 15, 2);
  tft.printf(time.c_str());
}

/********************************************************************
 * @brief     manage display blackout
 * @param[in] None
 * @return    None
 *********************************************************************/
void readBME280() {
  iRoom_temperature = bme.readTemperature();

  update_Room_temp();

  if (iRoom_temperature < iSet_temperature) {
    digitalWrite(RELAY_PIN, LOW);// Relay connected
  } else {
    digitalWrite(RELAY_PIN, HIGH);// Relay disconnected 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Az-Touch Mod 2.8 Display Thermostate");

  // Relay pinout
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay disconnected

  Serial.println(F("BME280 test"));
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  bool status;
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76, &I2CBME);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  initWiFi();
  timeClient.begin();
  initTouchScreen();

  IntroScreen();

  readBME280();

  interval0.interval(1000, updateTime);
  interval0.interval(10000, intervalBlackOut);
  interval0.interval(60000, readBME280);
  
}

void loop() {
  timeClient.update();
  interval0.handler();
  touchscreen();
}

