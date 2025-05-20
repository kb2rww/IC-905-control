// =======================================================================
// IC905_web_server_2.ino
// Main entry point for the IC-905 ESP32 Web Server Control project.
// Includes robust WiFi auto-reconnection logic.
// Robust WiFi, AP fallback, OLED status
// =======================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 // 32 or 64 depending on your screen
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include "WiFiSetup.h"    // Handles WiFi connection and configuration
#include "GpioControl.h"  // Handles GPIO pin management and HTTP parsing for GPIO
#include "WebPage.h"      // Handles dynamic web page generation for the UI

WiFiServer server(80);    // ESP32 web server on port 80

// Timeout management for web client
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000; // 2 seconds

void setup() {
  Serial.begin(115200);
  setupGPIO();

  // Initialize SSD1306 display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Halt if display doesn't initialize
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  setupWiFi();
  server.begin();
}

void loop() {
  // WiFi Auto-Reconnect Block
  if (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("WiFi lost!");
    display.println("Reconnecting...");
    display.display();

    // Keep trying to reconnect until successful
    while (WiFi.status() != WL_CONNECTED) {
      wifiMulti.run();
      delay(2000); // Wait 2 seconds between attempts
    }

    // Once reconnected
    Serial.println("WiFi reconnected!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("WiFi restored!");
    display.print("SSID: ");
    display.println(WiFi.SSID());
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
  }
  // --------------- Main Web Server Logic -------------------
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client found.");

    String header = "";
    String currentLine = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            handleGpioRequest(header);

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            sendHtmlPage(client);

            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
