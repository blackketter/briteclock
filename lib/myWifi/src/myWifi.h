#ifndef _myWifi_
#define _myWifi_
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* getHostname();
void setupWifi();
void loopWifi();

#endif
