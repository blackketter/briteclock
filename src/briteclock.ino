#include <Arduino.h>
#include <Switch.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "WiFiThing.h"
#include "Clock.h"
// create Credentials.h and define const char* ssid and passphrase
#include "Credentials.h"

// platformio seems to need these FIXME
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>

#define TFT_DC     2
#define TFT_CS     4
#define TFT_BL     0
#define TFT_RST    16
#define LIGHT_SENSOR (A0)

#define ANALOGRANGE (1024)
#define BUTTON_PIN (D1)

Switch button = Switch(BUTTON_PIN);  // Switch between a digital pin and GND

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

Clock clock;
Clock paris;

WiFiThing thing;
// WiFiConsole console is provided by WiFiThing

void setup() {

  thing.begin(ssid, passphrase);

  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0);

  tft.begin();
  tft.setRotation(3);

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

  paris.setZoneOffset(1*60*60);
  clock.setZoneOffset(-8*60*60);
}


void loop(void) {

  button.poll();

  thing.loop();

  static time_t lastTime = 0;
  static int fps = 0;
  static int lastfps = 0;

  bool redraw = false;

  // redraw every hour
  static int lastHour = 0;
  if (lastHour != clock.hour()) {
    lastHour = clock.hour();
    redraw = true;
  }

  if (Uptime::seconds() != lastTime) {
    lastTime = Uptime::seconds();
    char time[100];
    clock.longTime(time);
    console.debugln(time);
    console.debugf("%2d fps\n", fps);

    lastfps = fps;
    fps = 0;

  } else {
    fps++;
  }

  static bool screenoff = false;
  if (button.released() || button.longPress()) {
    redraw = true;
  }

  if (button.pushed()) {
    screenoff = !screenoff;
  }


  static uint32_t b = 0;
  uint32_t ambient = analogRead(LIGHT_SENSOR);
  b = (ambient*PWMRANGE)/ANALOGRANGE;
  if (b == 0) { b = 1; }
  if (screenoff && !button.on()) {
    b = 0;
  }

  analogWrite(TFT_BL, b);

  if (redraw) {
    tft.fillScreen(ILI9341_BLACK);
  }

  tft.setCursor(0, 0);
  tft.setTextSize(2);
  uint16_t c = ILI9341_RED;
  tft.setTextColor(c, ILI9341_BLACK);

  if (button.longPressLatch()) {
    // draw info
    tft.printf("Time: %2d:%02d:%02d.%01d\n",clock.hourFormat12(),clock.minute(),clock.second(),clock.fracMillis()/100);
    tft.printf("wifi: %s\n", WiFi.isConnected() ? "connected   " : "disconnected");
    tft.printf("uptime: %d\n", Uptime::seconds());

    String hostname = thing.getHostname();
    tft.printf("host:%s.local\n", hostname.c_str());
    String ip = thing.getIPAddress();
    tft.printf("ip: %s\n", ip.c_str());;

    tft.printf("ambient: %4d\n", ambient);
    tft.printf("brightness: %4d\n", b);

    static int16_t x = 0;
    static uint32_t lastmillis = millis();
    uint32_t ms = millis() - lastmillis;
    lastmillis = millis();

    tft.printf("fps: %2d (%4d)ms\n", lastfps, ms);

  } else {
    // draw clock
    if (clock.hasBeenSet()) {
      tft.printf("Paris: %d:%02d%s, %s", paris.hourFormat12(), paris.minute(), paris.isAM() ? "am":"pm", paris.weekdayString());
    } else if (!WiFi.isConnected()){
      tft.println("Connecting...");
    } else {
      tft.println("Trying to set time...");
    }

    tft.setTextSize(10);
    tft.println();
    if (clock.hasBeenSet()) {

      tft.printf("%2d:%02d",clock.hourFormat12(),clock.minute());
      tft.setTextSize(1);
      tft.printf("%02d",clock.second());
    }

    tft.setTextSize(10);
    tft.println();

    tft.setTextSize(2);

    if (clock.hasBeenSet()) {
      char longdate[100];
      sprintf(longdate, "%s, %s %d", clock.weekdayString(), clock.monthString(), clock.day());
      int spaces = (320/(2*6) - strlen(longdate))/2;
      for (int i = 0; i < spaces; i++) {
        tft.print(" ");
      }
      tft.print(longdate);
    }
  }
}


