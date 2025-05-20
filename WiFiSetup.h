#pragma once
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Adafruit_SSD1306.h>
extern Adafruit_SSD1306 display; // Declare display from main .ino

// List of SSIDs (WiFi network names) to try connecting to
const char* ssids[] = {"kb2rww", "kb2rwwp", "KB2RWW Silverado"};
const char* password = "1244600000";

// WiFiMulti object allows us to try multiple networks in order
WiFiMulti wifiMulti;

// Attempts to connect to any WiFi network in the list
void setupWiFi() {
  for (const char* ssid : ssids) {
    wifiMulti.addAP(ssid, password);
  }

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connected to SSID: ");
    Serial.println(WiFi.SSID());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("SSID: ");
    display.println(WiFi.SSID());
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
  }
  Serial.println("Connected. The device can be found at IP address:");
  Serial.println(WiFi.localIP());
}
