#include <Arduino.h>
#include <Switch.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "WiFiThing.h"
#include "Clock.h"
// create Credentials.h and define const char* ssid and passphrase
#include "Credentials.h"

#include "Wire.h"

// platformio seems to need these FIXME
#ifdef ESP32
#include <ESPmDNS.h>
//#include <ESPHTTPClient.h>
#else
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#endif
#include <NTPClient.h>
#include <ArduinoOTA.h>

#if defined(ESP32)
#define TFT_CS   15
#define TFT_DC   33
#define TFT_BL   22
#define TFT_RST    16

#define BUTTON_PIN 12

#define ANALOGRANGE (4096)
#define PWMRANGE (255)
#define WHITE_BRIGHTNESS (50)
#define LIGHT_SENSOR (34)

#else // esp8266
#define TFT_DC     2
#define TFT_CS     4
#define TFT_BL     0
#define TFT_RST    16

#define BUTTON_PIN (D1)

#define ANALOGRANGE (1024)
#define WHITE_BRIGHTNESS (500)
#define LIGHT_SENSOR (A0)
#endif


Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

Clock localTime;
Clock parisTime;
Clock easternTime;

WiFiThing thing;
// WiFiConsole console is provided by WiFiThing

const int ledChannel = 0;

void setupBacklight() {
#if defined(ESP32)
  int freq = 5000;
  int resolution = 8;
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(TFT_BL, ledChannel);
#else
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0);
  pinMode(LIGHT_SENSOR, ANALOG);
#endif
}

void setBacklight(uint8_t b) {
#if defined(ESP32)
  ledcWrite(ledChannel, b);
#else
  analogWrite(TFT_BL, b);
#endif
}

void setup() {

  thing.begin(ssid, passphrase);

  setupBacklight();
  tft.begin();
  tft.setRotation(3);

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  console.debugf("Display Power Mode: 0x%x\n", x);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  console.debugf("MADCTL Mode: 0x%x\n", x);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  console.debugf("Pixel Format: 0x%x\n", x);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  console.debugf("Image Format: 0x%x\n", x);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  console.debugf("Self Diagnostic: 0x%x\n", x);

  tft.fillScreen(ILI9341_BLACK);

//  parisTime.setZone(&CE);
  easternTime.setZone(&usET);
  localTime.setZone(&usPT);
}



void loop(void) {

  button.poll();

  thing.idle();

  static time_t lastTime = 0;
  static int fps = 0;
  static int lastfps = 0;

  bool redraw = false;

  // redraw every hour
  static int lastHour = 0;
  if (lastHour != localTime.hour()) {
    lastHour = localTime.hour();
    redraw = true;
  }

  if (Uptime::seconds() != lastTime) {
    lastTime = Uptime::seconds();
    lastfps = fps;
    fps = 0;

  } else {
    fps++;
  }

  static bool screenoff = false;
  static bool toggleInfo =  false;
if (button.released() || button.longPress()) {
    redraw = true;
  }

  if (button.pushed()) {
    if (toggleInfo) {
      toggleInfo = false;
      screenoff = false;
    } else {
      screenoff = !screenoff;
    }
  }

  if (button.longPress()) {
    toggleInfo = true;
    screenoff = false;
  }

  static uint32_t b = 128;
  uint32_t ambient = analogRead(LIGHT_SENSOR);

  b = (ambient*PWMRANGE)/ANALOGRANGE;
  if (b == 0) { b = 1; }
  if (screenoff && !button.on()) {
    b = 0;
  }

  setBacklight(b);

  if (redraw) {
    tft.fillScreen(ILI9341_BLACK);
  }

  tft.setCursor(0, 0);
  tft.setTextSize(2);
  uint16_t c;

  if (b > WHITE_BRIGHTNESS) {
    c = ILI9341_WHITE;
  } else {
    c = ILI9341_RED;
  }

  tft.setTextColor(c, ILI9341_BLACK);



  if (toggleInfo) {
    // draw info
    tft.print("Date: ");
    localTime.shortDate(tft);
    tft.print("\nTime: ");
    localTime.longTime(tft);
    tft.printf(".%01d\n",localTime.fracMillis()/100);

    tft.printf("Offset: %d\n", localTime.getZoneOffset());

    tft.printf("wifi: %s\n", WiFi.isConnected() ? "connected   " : "disconnected");
    tft.printf("uptime: %d\n", (int)Uptime::seconds());

    String hostname = thing.getHostname();
    tft.printf("host:%s.local\n", hostname.c_str());
    String ip = thing.getIPAddress();
    tft.printf("ip: %s\n", ip.c_str());;

    tft.printf("ambient: %4d\n", ambient);
    tft.printf("brightness: %4d\n", b);

    static uint32_t lastmillis = millis();
    uint32_t ms = millis() - lastmillis;
    lastmillis = millis();

    tft.printf("fps: %2d (%4d)ms\n", lastfps, ms);
    screenoff = false;

  } else if (b) {
    // draw clock
    if (localTime.hasBeenSet()) {
      const char* abbrev = "???";
      TimeChangeRule* rule = easternTime.getZoneRule();

      if (rule) {
        abbrev = rule->abbrev;
      }

      tft.printf("%s: %d:%02d%s, %s", abbrev, easternTime.hourFormat12(), easternTime.minute(), easternTime.isAM() ? "am":"pm", easternTime.weekdayString());
    } else if (!WiFi.isConnected()){
      tft.println("Connecting...");
    } else {
      tft.println("Trying to set time...");
    }

    tft.setTextSize(10);
    tft.println();
    if (localTime.hasBeenSet()) {

      tft.printf("%2d:%02d",localTime.hourFormat12(),localTime.minute());
      tft.setTextSize(1);
      tft.printf("%02d",localTime.second());
    }

    tft.setTextSize(10);
    tft.println();

    tft.setTextSize(2);

    if (localTime.hasBeenSet()) {
      char longdate[100];
      sprintf(longdate, "%s, %s %d", localTime.weekdayString(), localTime.monthString(), localTime.day());
      int spaces = (320/(2*6) - strlen(longdate))/2;
      for (int i = 0; i < spaces; i++) {
        tft.print(" ");
      }
      tft.print(longdate);
    }
  } else {
    delay(10);  // todo: if I don't include this, the wifi disconnects.  delay(1) doesn't work, nor does yield()  wierd
  }
}



