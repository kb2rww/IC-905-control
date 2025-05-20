// =======================================================================
// IC905_web_server_2.ino
// Main entry point for the IC-905 ESP32 Web Server Control project.
// Now includes robust WiFi auto-reconnection in the main loop.
// =======================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include "WiFiSetup.h"    // Handles WiFi connection and configuration, including multi-SSID attempts
#include "GpioControl.h"  // Handles GPIO pin management and HTTP parsing for GPIO
#include "WebPage.h"      // Handles dynamic web page generation for the UI

WiFiServer server(80);    // Create an instance of the ESP32 web server on port 80

// Variables used for timeout management
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000; // Timeout duration (2 seconds)

void setup() {
  Serial.begin(115200);
  setupGPIO();            // Initialize all GPIO pins as outputs and set them to LOW ("off")

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

  setupWiFi();            // Attempt to connect to available WiFi networks. Only runs once here.
  server.begin();         // Start the web server to listen for incoming clients
}

// =======================================================================
// Main loop - now includes WiFi auto-reconnect logic
// =======================================================================
void loop() {
  // ---------------------------------------------------------------------
  // WiFi Auto-Reconnect Block
  // ---------------------------------------------------------------------
  // If WiFi connection is lost, attempt to reconnect.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost. Attempting to reconnect...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("WiFi lost!");
    display.println("Reconnecting...");
    display.display();

    // WiFiMulti.run() will try all SSIDs added, and only returns WL_CONNECTED if successful
    if (wifiMulti.run() == WL_CONNECTED) {
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
    } else {
      // Still not connected, keep trying every second
      delay(1000);
      return; // Skip rest of loop until WiFi is restored
    }
  }

  // ---------------------------------------------------------------------
  // Main Web Server Logic (unchanged)
  // ---------------------------------------------------------------------
  WiFiClient client = server.available();
  if (client) {  // If a client is connected
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client found.");

    String header = "";
    String currentLine = "";

    // Stay in this loop while the client is connected and we haven't timed out
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
