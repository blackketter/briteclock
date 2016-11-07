#include <Arduino.h>
#include <Switch.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TimeLib.h"
#include "myWifi.h"
#include "WiFiConsole.h"
#include "Clock.h"

WiFiConsole console;

#define TFT_DC     2
#define TFT_CS     4
#define TFT_BL     0
#define LIGHT_SENSOR (A0)

#define BUTTON_PIN (D1)
Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

Clock clock = Clock();

// todo - make this configurable

void setup() {

  setupWifi("briteclock", -7*60*60);

  console.begin();

  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 1);

  tft.begin();
  tft.setRotation(1);

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  console.print("Display Power Mode: 0x"); console.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  console.print("MADCTL Mode: 0x"); console.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  console.print("Pixel Format: 0x"); console.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  console.print("Image Format: 0x"); console.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  console.print("Self Diagnostic: 0x"); console.println(x, HEX);

  tft.fillScreen(ILI9341_BLACK);
}


void loop(void) {
  console.loop();

//  console.debugln("polling");
  button.poll();

//  console.debugln("wifi");
  loopWifi();

  static time_t lastTime = 0;
  static int fps = 0;
  static int lastfps = 0;
  if (now() != lastTime) {
    lastTime = now();
    char time[100];
    clock.longTime(time);
    console.debugln(time);
    console.debugf("%2d fps\n", fps);

    lastfps = fps;
    fps = 0;

  } else {
    fps++;
  }

  static uint32_t b = 0;

  if (button.pushed()) {
  }

  int h = hour();
  int m = minute();
  if (h > 12) { h -= 12; }
  if (h == 0) { h = 12; }

  tft.setCursor(0, 0);
  tft.setTextSize(9);
  uint16_t c = ILI9341_RED;
  tft.setTextColor(c, ILI9341_BLACK);
  tft.printf("\n%2d:%02d",h,m);

  tft.setTextSize(2);
  tft.printf("%02d",second());

  tft.setTextSize(9);
  tft.println();

  tft.setTextSize(2);

  tft.printf("wifi: %d\n", WiFi.isConnected());
  uint32_t ambient = analogRead(LIGHT_SENSOR);
  tft.printf("ambient: %4d\n", ambient);
  b = PWMRANGE - ambient;
  analogWrite(TFT_BL, b);
  tft.printf("brightness: %4d\n", b);

     static int16_t x = 0;
    static uint32_t lastmillis = millis();
    uint32_t ms = millis() - lastmillis;
    lastmillis = millis();

    tft.printf("fps: %2d (%4d)ms\n", lastfps, ms);
//  console.debugln("done");

    const int maxheight = 30;
    const int maxmillis = 500;
    uint16_t height = (ms * maxheight)/ maxmillis;
    if (height > maxheight) { height = maxheight; }
    if (ms > 49) { ms = 49; }
    tft.drawLine(x,239-height,x,239-maxheight,ILI9341_BLACK);
    tft.drawLine(x,239,x,239-height, ILI9341_BLUE);
    x++;
    if (x>319) { x = 0; }


}


