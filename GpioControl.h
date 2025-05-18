// =======================================================================
// GpioControl.h
// Defines the GPIO pins, their labels, and their ON/OFF state.
// Includes functions for GPIO initialization and HTTP command parsing.
// =======================================================================

#pragma once

// GpioControl struct represents each controllable GPIO pin
struct GpioControl {
  int pin;           // The pin number on the ESP32 (e.g., 4, 16, etc.)
  const char* label; // Human-readable name to display on the web page
  String state;      // Holds "on" or "off" to represent the current state
};

// List of all GPIOs you want to control via the web page
GpioControl controls[] = {
  {0,  "Motor forward",     "off"},
  {4,  "Motor reverse",     "off"},
  {32, "144 to 1296 triband", "off"},
  {25, "2304Ghz",           "off"},
  {27, "Omnie enable",      "off"},
  {16, "5760Ghz",           "off"},
  {17, "triband dish",      "off"},
  {21, "IO21",              "off"},
  {22, "IO22",              "off"},
  {2,  "LED",               "off"}
};
// Calculate the total number of GPIO controls (for use in loops)
const size_t CONTROL_COUNT = sizeof(controls) / sizeof(controls[0]);

// Initializes each GPIO pin as an output and sets it LOW (off)
void setupGPIO() {
  for (size_t i = 0; i < CONTROL_COUNT; i++) {
    pinMode(controls[i].pin, OUTPUT);      // Set as output pin
    digitalWrite(controls[i].pin, LOW);    // Ensure output starts LOW (off)
    controls[i].state = "off";             // Initialize state string
  }
}

// Parses the HTTP request header for any GPIO ON/OFF commands
// If a command is found, set the pin HIGH/LOW and update the state
void handleGpioRequest(const String& header) {
  for (size_t i = 0; i < CONTROL_COUNT; i++) {
    // Build the ON and OFF commands (e.g., "GET /4/on")
    String onCmd  = "GET /" + String(controls[i].pin) + "/on";
    String offCmd = "GET /" + String(controls[i].pin) + "/off";
    // If ON command found in HTTP header, set the pin HIGH and mark as "on"
    if (header.indexOf(onCmd) >= 0) {
      Serial.printf("GPIO %d ON\n", controls[i].pin);
      controls[i].state = "on";
      digitalWrite(controls[i].pin, HIGH);
    }
    // If OFF command found, set the pin LOW and mark as "off"
    else if (header.indexOf(offCmd) >= 0) {
      Serial.printf("GPIO %d OFF\n", controls[i].pin);
      controls[i].state = "off";
      digitalWrite(controls[i].pin, LOW);
    }
  }
}