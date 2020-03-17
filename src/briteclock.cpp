// this needs to be included early to fix a build problem
#include "FS.h"

#include <Arduino.h>
#include <Switch.h>

#include "TFT_eSPI.h"
#include "NullStream.h"
#include "WiFiThing.h"
#include "Clock.h"
// create Credentials.h and define const char* ssid and passphrase
#include "Credentials.h"

uint32_t ambient;
uint32_t brightness = 128;

#include "Commands/FPSCommand.h"
FPSCommand theFPSCommand;

class ScreenCommand : public Command {
  public:
    const char* getName() { return "screen"; }
    const char* getHelp() { return ("display screen info"); }
    void execute(Console* c, uint8_t paramCount, char** params) {
      c->printf("Ambient: %4d\n", ambient);
      c->printf("Brightness: %4d\n", brightness);
      c->printf("FPS: %3.2f\n", theFPSCommand.lastFPS());
    }
};
ScreenCommand theScreenCommand;

#if defined(ESP32)

  #define BUTTON_PIN 12

  #define ANALOGRANGE (4096)

  #define PWMRESOLUTION (10)
const uint32_t  analogWriteMax = 1023;

  #define WHITE_BRIGHTNESS (50)
  #define LIGHT_SENSOR (34)

#else // esp8266
//  #define TFT_DC     2
//  #define TFT_CS     4
//  #define TFT_BL     0
//  #define TFT_RST    16

  #define BUTTON_PIN (D1)

  // NOTE: esp8266 pwm calculations are done in microseconds, so make sure things are in power of 10 units
const uint32_t  analogWriteMax = 1000;
  #define ANALOGRANGE (1024)
  #define WHITE_BRIGHTNESS (500)
  #define LIGHT_SENSOR (A0)
#endif

Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

//#if defined(ESP32)
TFT_eSPI tft = TFT_eSPI();

//#else
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
//#endif

Clock localTime;
Clock otherTime;

WiFiThing thing;
// WiFiConsole console is provided by WiFiThing

const int ledChannel = 0;

void setupBacklight() {
#if defined(ESP32)
  int freq = 5000;
  ledcSetup(ledChannel, freq, PWMRESOLUTION);
  ledcAttachPin(TFT_BL, ledChannel);
#else
  pinMode(TFT_BL, OUTPUT);
//  pinMode(LIGHT_SENSOR, ANALOG);

  analogWriteRange(analogWriteMax);
  analogWrite(TFT_BL, 0);
#endif
}

void setBacklight(uint16_t b) {
#if defined(ESP32)
  ledcWrite(ledChannel, analogWriteMax-b);
#else
  analogWrite(TFT_BL, b);
#endif
}

void setup() {
  delay(1000);
  thing.setTimezone(&usPT);
  thing.setHostname(HOSTNAME);

  thing.begin(ssid, passphrase);

  setupBacklight();
  tft.begin();
  tft.setRotation(3);

  otherTime.setZone(&usET);
  localTime.setZone(&usPT);
}

uint32_t calculateBrightness(uint32_t light) {

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

  uint32_t brightness = (lightAve*analogWriteMax)/ANALOGRANGE;

  if (brightness == 0) { brightness = 1; }

	if (brightness < 5) { brightness = 5; }
	
  return brightness;
}

void loop(void) {

  button.poll();
  thing.idle();

  static time_t lastTime = 0;

  bool redraw = false;
  bool update = false;

  // redraw every hour
  static int lastHour = 100;
  if (lastHour != localTime.hour()) {
    lastHour = localTime.hour();
    redraw = true;
  }

  if (Uptime::seconds() != lastTime) {
    lastTime = Uptime::seconds();
    update = true;
  }

  static bool screenoff = false;
  static bool showinfo =  false;

  if (button.pushed()) {
    if (showinfo) {
      showinfo = false;
      screenoff = false;
    } else {
      screenoff = !screenoff;
    }
    redraw = true;
  }

  if (button.longPress()) {
    showinfo = true;
    screenoff = false;
    redraw = true;
  }

  ambient = analogRead(LIGHT_SENSOR);

  brightness = calculateBrightness(ambient);

  if (screenoff) {
    brightness = 0;
  }

  setBacklight(brightness);

  if (redraw) {
    tft.fillScreen(TFT_BLACK);
  }

  uint16_t c;

  if (brightness > WHITE_BRIGHTNESS) {
    c = TFT_WHITE;
  } else {
    c = TFT_RED;
  }

  tft.setTextColor(c, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(0, 0 /*tft.fontHeight()*/);

  if (showinfo) {
    tft.setTextFont(2);
    tft.setCursor(0,0);
    // commands take Streams, but tft is a Print, so we need an adapter.
    PrintStream tftstream(&tft);
    console.executeCommandLine(&tftstream, "info");
    console.executeCommandLine(&tftstream, "screen");
    tftstream.print("Hostname: ");
    tftstream.println(thing.getHostname());
		tftstream.print("IP: ");
    tftstream.println(thing.getIPAddress());

    theFPSCommand.newFrame();
  } else if (brightness && update) {
    // draw clock
    if (localTime.hasBeenSet()) {
      const char* abbrev = "???";
      TimeChangeRule* rule = otherTime.getZoneRule();

      if (rule) {
        abbrev = rule->abbrev;
      }
      tft.setTextFont(4);
      tft.printf("%s: %d:%02d%s, %s", abbrev, otherTime.hourFormat12(), otherTime.minute(), otherTime.isAM() ? "am":"pm", otherTime.weekdayString());
    } else if (!WiFi.isConnected()){
      tft.println("Connecting...");
    } else {
      tft.println("Trying to set time...");
    }

      tft.setTextFont(8);
        char timeStr[6];

    if (localTime.hasBeenSet()) {
      sprintf(timeStr,"%d:%02d",localTime.hourFormat12(),localTime.minute());
      tft.setCursor(160-tft.textWidth(timeStr)/2,120-tft.fontHeight()/2);
      tft.print(timeStr);
      tft.setTextFont(2);
      tft.printf("%02d",localTime.second());
    }

    tft.setTextFont(8);
    tft.println();
    tft.setTextFont(4);
    tft.println();

    if (localTime.hasBeenSet()) {
      char longdate[100];
      sprintf(longdate, "%s, %s %d", localTime.weekdayString(), localTime.monthString(), localTime.day());
      tft.setCursor(160-tft.textWidth(longdate)/2, tft.getCursorY());

      tft.print(longdate);
      theFPSCommand.newFrame();
    }
  }
}



