// =======================================================================
// IC905_web_server.ino
// Main entry point for the IC-905 ESP32 Web Server Control project.
//
// This file sets up the system, starts the web server, and contains the
// main loop which handles incoming HTTP client connections.
// =======================================================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#include "WiFiSetup.h"    // Handles WiFi connection and configuration
#include "GpioControl.h"  // Handles GPIO pin management and HTTP parsing for GPIO
#include "WebPage.h"      // Handles dynamic web page generation for the UI
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create an instance of the ESP32 web server on port 80 (HTTP default)
WiFiServer server(80);

// OLED display width and height, adjust if your display is different
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  // or 64 for 128x64 displays

// Declare display object (I2C address is usually 0x3C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variables used for timeout management
unsigned long currentTime = millis();    // Tracks current time in milliseconds
unsigned long previousTime = 0;          // Stores time of last client activity
const long timeoutTime = 2000;           // Timeout duration (2 seconds)

void setup() {
  Serial.begin(115200);   // Start serial communication for debugging (baud rate 115200)
  setupGPIO();            // Initialize all GPIO pins as outputs and set them to LOW ("off")
  // Initialize SSD1306 display
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is the default I2C address
  Serial.println(F("SSD1306 allocation failed"));
  for (;;); // Halt if display doesn't initialize
}
display.clearDisplay();
display.setTextSize(1);      // Normal 1:1 pixel scale
display.setTextColor(SSD1306_WHITE);
display.setCursor(0,0);
display.println("Starting...");
display.display();
  setupWiFi();            // Attempt to connect to available WiFi networks
  server.begin();         // Start the web server to listen for incoming clients
}

void loop() {
  // Check if a new client has connected to the server
  WiFiClient client = server.available();
  if (client) {  // If a client is connected
    currentTime = millis();           // Update the current time
    previousTime = currentTime;       // Set previous time to now (reset timeout)
    Serial.println("New Client found.");

    String header = "";               // Will hold the full HTTP request header
    String currentLine = "";          // Used to capture each line of the request

    // Stay in this loop while the client is connected and we haven't timed out
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();         // Update current time for timeout checking

      if (client.available()) {       // If data is available from the client
        char c = client.read();       // Read one byte from the client
        Serial.write(c);              // Echo the byte to the serial monitor for debugging
        header += c;                  // Append the byte to the header string

        // If the character is a newline, we've reached the end of a line
        if (c == '\n') {
          // If the current line is empty, we've gotten two newlines in a row: end of HTTP request
          if (currentLine.length() == 0) {
            // Handle any GPIO toggle commands requested by the client
            handleGpioRequest(header);

            // Send the HTTP response headers
            client.println("HTTP/1.1 200 OK");             // Response code 200 = OK
            client.println("Content-type:text/html");       // Indicate that we are sending HTML
            client.println("Connection: close");            // Tell the client to close the connection after response
            client.println();

            // Generate and send the HTML page with current GPIO states
            sendHtmlPage(client);

            client.println();                               // Extra newline to end the response
            break;                                          // Break out of the while loop (done with request)
          } else {
            currentLine = "";                               // Reset currentLine for the next line
          }
        } else if (c != '\r') {
          currentLine += c;                                 // If not a carriage return, append character to currentLine
        }
      }
    }

    client.stop();                                          // Terminate the connection with the client
    Serial.println("Client disconnected.");                 // Notify via serial
    Serial.println("");
  }
}
