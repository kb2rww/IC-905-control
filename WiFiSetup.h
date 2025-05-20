// =======================================================================
// WiFiSetup.h
// Handles WiFi network configuration and connection attempts.
//
// This file creates a list of possible WiFi networks, and attempts to
// connect to one of them using the same password.
// =======================================================================

#pragma once
#include <WiFi.h>         // ESP32 WiFi library
#include <WiFiMulti.h>    // Allows cycling through multiple WiFi networks
#include <Adafruit_SSD1306.h>
extern Adafruit_SSD1306 display; // Declare display from main .ino

// List of SSIDs (WiFi network names) to try connecting to
const char* ssids[] = {"kb2rww", "kb2rwwp", "KB2RWW Silverado"};
// WiFi password (must match all listed SSIDs)
const char* password = "1244600000";

// WiFiMulti object allows us to try multiple networks in order
WiFiMulti wifiMulti;

// Attempts to connect to any WiFi network in the list
void setupWiFi() {
  // Add each SSID to WiFiMulti with the shared password
  for (const char* ssid : ssids) {
    wifiMulti.addAP(ssid, password);
  }

  Serial.println("Connecting Wifi...");
  // Try to connect to any available network in the list
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("WiFi connected");           // Connection successful
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());             // Print assigned IP
    Serial.print("Connected to SSID: ");
    Serial.println(WiFi.SSID());                // Print SSID
  
    // --- Add this block to display SSID and IP ---
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("SSID: ");
  display.println(WiFi.SSID());
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  }
  // Print final connection status and device IP for user reference
  Serial.println("Connected. The device can be found at IP address:");
  Serial.println(WiFi.localIP());
}
