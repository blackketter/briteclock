#include <Arduino.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Switch.h"

#include "myWifi.h"

#define TFT_DC     2
#define TFT_CS     4
#define TFT_BL     0
#define LIGHT_SENSOR (A0)

#define BUTTON_PIN (D1)
Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);


// todo - make this configurable

void setup() {
  Serial.begin(9600);
  Serial.println("ILI9341 Test!");

  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 1);

  tft.begin();

  setupWifi();


  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);

  testText();

}


void loop(void) {
  button.poll();
  loopWifi();

  static uint32_t b = PWMRANGE;

  if (button.pushed()) {
    analogWrite(TFT_BL, ~b);
    Serial.println(b);
    if (b == 0) {
      b = PWMRANGE;
    } else {
      b=b>>1;
    }
  }

  Serial.printf("texttime: %dms\n",testText()/1000);
  Serial.printf("brightness: %d\n", b);
  tft.printf("brightness: %d\n", b);
  uint32_t ambient = analogRead(LIGHT_SENSOR);
  Serial.printf("ambient: %d\n", ambient);
  tft.printf("ambient: %d\n", ambient);

}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.fillScreen(ILI9341_RED);
  yield();
  tft.fillScreen(ILI9341_GREEN);
  yield();
  tft.fillScreen(ILI9341_BLUE);
  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}

unsigned long testText() {
//  tft.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}

