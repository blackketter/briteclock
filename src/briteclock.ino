#include <Arduino.h>
#include <Switch.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TimeLib.h"
#include "myWifi.h"
#include "WiFiConsole.h"

WiFiConsole console;

#define TFT_DC     2
#define TFT_CS     4
#define TFT_BL     0
#define LIGHT_SENSOR (A0)

#define BUTTON_PIN (D1)
Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);


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
  button.poll();
  console.loop();
  loopWifi();

  static uint32_t b = PWMRANGE;

  if (button.pushed()) {
    if (b == 0) {
      b = PWMRANGE;
    } else {
      b=b>>1;
    }
//    analogWrite(TFT_BL, ~b);
    console.printf("brightness: %4d\n\n", b);
  }

  int h = hour();
  int m = minute();
  if (h > 12) { h -= 12; }
  if (h == 0) { h = 12; }

  tft.setCursor(0, 0);
  tft.setTextSize(9);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.printf("\n%2d:%02d",h,m);

  tft.setTextSize(2);
  tft.printf("%02d",second());

  tft.setTextSize(9);
  tft.println();

  tft.setTextSize(2);

  tft.printf("brightness: %4d\n", b);

  uint32_t ambient = analogRead(LIGHT_SENSOR);
  tft.printf("ambient: %4d\n\n", ambient);
  analogWrite(TFT_BL, ~ambient);
}


