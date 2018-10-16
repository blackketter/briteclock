#include <Arduino.h>
#include <Switch.h>

#include "SPI.h"
#include "TFT_eSPI.h"
#if defined(ESP32)
#else
//#include "Adafruit_GFX.h"
//#include "Adafruit_ILI9341.h"
#endif


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

#include "Commands/FPSCommand.h"
FPSCommand theFPSCommand;

#if defined(ESP32)

	#define BUTTON_PIN 12

	#define ANALOGRANGE (4096)
	#define PWMRANGE (1023)
	#define WHITE_BRIGHTNESS (50)
	#define LIGHT_SENSOR (34)

#else // esp8266
//	#define TFT_DC     2
//	#define TFT_CS     4
//	#define TFT_BL     0
//	#define TFT_RST    16

	#define BUTTON_PIN (D1)

	#define ANALOGRANGE (1024)
	#define WHITE_BRIGHTNESS (500)
	#define LIGHT_SENSOR (A0)
	#define TFT_BLACK ILI9341_BLACK
	#define TFT_WHITE ILI9341_WHITE
	#define TFT_RED ILI9341_RED
#endif

Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

//#if defined(ESP32)
TFT_eSPI tft = TFT_eSPI();

//#else
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
//#endif

Clock localTime;
Clock easternTime;

WiFiThing thing;
// WiFiConsole console is provided by WiFiThing

const int ledChannel = 0;

void setupBacklight() {
#if defined(ESP32)
  int freq = 5000;
  int resolution = 10;
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(TFT_BL, ledChannel);
#else
  pinMode(TFT_BL, OUTPUT);
//  pinMode(LIGHT_SENSOR, ANALOG);

  analogWriteRange(PWMRANGE);
  analogWrite(TFT_BL, 0);
#endif
}

void setBacklight(uint16_t b) {
#if defined(ESP32)
  ledcWrite(ledChannel, PWMRANGE-b);
#else
  analogWrite(TFT_BL, b);
#endif
}

void setup() {

  thing.begin(ssid, passphrase);

  setupBacklight();
  tft.begin();
  tft.setRotation(3);

#if 0
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
#endif

  tft.fillScreen(TFT_BLACK);

  easternTime.setZone(&usET);
  localTime.setZone(&usPT);
}

uint32_t updateBrightness(uint32_t light) {

  const uint8_t historyLen = 8;
  static uint8_t last = 0;
  static uint32_t lightHistory[historyLen];

  lightHistory[last] = light;
  last++;
  if (last >= historyLen) {
    last = 0;
  }

  uint32_t lightAve = 0;
  for (uint8_t i = 0; i<historyLen; i++) {
    lightAve += lightHistory[i];
  }
  lightAve /= historyLen;

  uint32_t brightness = (lightAve*PWMRANGE)/ANALOGRANGE;

  if (brightness == 0) { brightness = 1; }

  return brightness;
}

void loop(void) {

  button.poll();

  thing.idle();

  static time_t lastTime = 0;

  bool redraw = false;

  // redraw every hour
  static int lastHour = 0;
  if (lastHour != localTime.hour()) {
    lastHour = localTime.hour();
    redraw = true;
  }

  if (Uptime::seconds() != lastTime) {
    lastTime = Uptime::seconds();
  }

  static bool screenoff = false;
  static bool toggleInfo =  false;

  if (button.pushed()) {
    if (toggleInfo) {
      toggleInfo = false;
      screenoff = false;
    } else {
      screenoff = !screenoff;
    }
    redraw = true;
  }

  if (button.longPress()) {
    toggleInfo = true;
    screenoff = false;
    redraw = true;
  }

  static uint32_t b = 128;
  uint32_t ambient = analogRead(LIGHT_SENSOR);

  b = updateBrightness(ambient);

  if (screenoff) {
    b = 0;
  }

  setBacklight(b);

  if (redraw) {
    tft.fillScreen(TFT_BLACK);
  }

  uint16_t c;

  if (b > WHITE_BRIGHTNESS) {
    c = TFT_WHITE;
  } else {
    c = TFT_RED;
  }

  tft.setTextColor(c, TFT_BLACK);
  tft.setTextFont(2);
//  tft.setTextSize(2);
//  tft.setFreeFont(&FreeSans9pt7b);
  tft.setCursor(0, 0 /*tft.fontHeight()*/);

  if (toggleInfo) {
  	tft.setTextFont(2);
  	tft.setCursor(0,0);
    // draw info
    tft.print("Date: ");
    localTime.shortDate(tft);
    tft.print("\nTime: ");
    localTime.longTime(tft);
    tft.printf(".%01d\n",localTime.fracMillis()/100);

    tft.printf("Offset: %d\n", localTime.getZoneOffset());

    tft.printf("WiFi is %s\n", WiFi.isConnected() ? "connected   " : "disconnected");
    tft.printf("Uptime: %d\n", (int)Uptime::seconds());

    String hostname = thing.getHostname();
    tft.printf("Hostname: %s.local\n", hostname.c_str());
    String ip = thing.getIPAddress();
    tft.printf("IP: %s\n", ip.c_str());;

    tft.printf("Ambient: %4d\n", ambient);
    tft.printf("Brightness: %4d\n", b);
    tft.printf("FPS: %3.2f\n", theFPSCommand.lastFPS());

    screenoff = false;
    theFPSCommand.newFrame();
  } else if (b) {
    // draw clock
    if (localTime.hasBeenSet()) {
      const char* abbrev = "???";
      TimeChangeRule* rule = easternTime.getZoneRule();

      if (rule) {
        abbrev = rule->abbrev;
      }
	  tft.setTextFont(2);
      tft.printf("%s: %d:%02d%s, %s", abbrev, easternTime.hourFormat12(), easternTime.minute(), easternTime.isAM() ? "am":"pm", easternTime.weekdayString());
    } else if (!WiFi.isConnected()){
      tft.println("Connecting...");
    } else {
      tft.println("Trying to set time...");
    }

//    tft.setTextSize(10);
      tft.setTextFont(8);
//    tft.println();
        char timeStr[6];

    if (localTime.hasBeenSet()) {
      sprintf(timeStr,"%d:%02d",localTime.hourFormat12(),localTime.minute());
      tft.setCursor(160-tft.textWidth(timeStr)/2,120-tft.fontHeight()/2);
      tft.print(timeStr);
//      tft.setTextSize(1);
      tft.setTextFont(2);
      tft.printf("%02d",localTime.second());
    }

//    tft.setTextSize(10);
    tft.setTextFont(8);
    tft.println();
    tft.setTextFont(4);
    tft.println();

//    tft.setTextSize(2);

    if (localTime.hasBeenSet()) {
      char longdate[100];
      sprintf(longdate, "%s, %s %d", localTime.weekdayString(), localTime.monthString(), localTime.day());
      tft.setCursor(160-tft.textWidth(longdate)/2, tft.getCursorY());

/*
      int spaces = (320/(2*6) - strlen(longdate))/2;
      for (int i = 0; i < spaces; i++) {
        tft.print(" ");
      }
*/
      tft.print(longdate);
      theFPSCommand.newFrame();
    }
  } else {
    delay(10);  // todo: if I don't include this, the wifi disconnects.  delay(1) doesn't work, nor does yield()  wierd
  }
}



