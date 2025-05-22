// === INCLUDE ALL NECESSARY LIBRARIES ===
#include <U8g2lib.h>                // OLED display library for drawing graphics/text
#include <SPI.h>                    // SPI library (used for OLED hardware interface)
#include <BLEDevice.h>              // ESP32 BLE stack main library
#include <BLEUtils.h>               // BLE utility functions
#include <BLEScan.h>                // BLE scanning functionality (find devices)
#include <BLEAdvertisedDevice.h>    // BLE advertisement data handling

// === OLED SPI PIN DEFINITIONS ===
// These are the ESP32 pins wired to the OLED display module
#define OLED_CS    5                // Chip Select for SPI OLED
#define OLED_DC    17               // Data/Command select for SPI OLED
#define OLED_RESET 16               // Reset signal for SPI OLED

// === CREATE OLED DISPLAY OBJECT ===
// This object handles all drawing to the SSD1309 128x64 OLED via hardware SPI
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(
  U8G2_R0,                         // No screen rotation
  OLED_CS,                         // Chip Select pin
  OLED_DC,                         // Data/Command pin
  OLED_RESET                       // Reset pin
);

// === BLE SERVICE AND CHARACTERISTIC UUIDS (MUST MATCH SERVER) ===
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0" // Main BLE service
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1" // Notification char
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2" // Write char

// === BLE CLIENT STATE VARIABLES ===
static boolean doConnect = false;                  // Flag: true when a scan finds the right device
static BLEAdvertisedDevice* myDevice = nullptr;    // Pointer for the found BLE device
BLEClient* pClient = nullptr;                      // BLE Client connection object
BLERemoteCharacteristic* pStatusCharacteristic = nullptr;  // For notifications from server
BLERemoteCharacteristic* pCommandCharacteristic = nullptr; // For sending commands to server

// === BUTTON GPIO ASSIGNMENTS (6 BUTTONS) AND THEIR BLE COMMANDS ===
#define BUTTON1_PIN 4      // Button 1 wired to GPIO4
#define BUTTON2_PIN 14      // Button 2 wired to GPIO8
#define BUTTON3_PIN 12     // Button 3 wired to GPIO12
#define BUTTON4_PIN 32     // Button 4 wired to GPIO32
#define BUTTON5_PIN 25     // Button 5 wired to GPIO25
#define BUTTON6_PIN 27     // Button 6 wired to GPIO27

// -- Array of the GPIO pins for buttons, for looping and handling
const int BUTTON_PINS[] = {
  BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN, BUTTON5_PIN, BUTTON6_PIN
};
// -- Array of command strings for each button, in same order as pins
const char* BUTTON_COMMANDS[] = {
  "control power", // Button 1 (GPIO4)
  "motor left",    // Button 2 (GPIO14)
  "motor right",   // Button 3 (GPIO12)
  "relay 1",       // Button 4 (GPIO32)
  "relay 2",       // Button 5 (GPIO25)
  "relay 3"        // Button 6 (GPIO27)
};
const int NUM_BUTTONS = 6;    // Number of buttons in use

// === BUTTON DEBOUNCE STATE VARIABLES ===
// -- Tracks last debounce time for each button
unsigned long lastDebounceTime[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
// -- Last stable state (HIGH for not pressed due to INPUT_PULLUP)
bool lastButtonState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
// -- Whether button is currently registered as pressed (for edge detection)
bool buttonPressed[NUM_BUTTONS] = {false, false, false, false, false, false};
// -- Minimum ms for a stable press (debounce)
const unsigned long debounceDelay = 50;

// === VISUAL STATUS FOR BOXES (FILLED WHEN "ON" FROM SERVER, EMPTY OTHERWISE) ===
bool buttonStates[NUM_BUTTONS] = {false, false, false, false, false, false}; // false=off, true=on

// === BLE SCAN CALLBACK HANDLER ===
// This class is used to process BLE advertisements during scanning
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  // This function is called on each BLE advertisement packet
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Check if the advertised device offers the target service UUID
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      myDevice = new BLEAdvertisedDevice(advertisedDevice); // Copy device info
      doConnect = true;                                     // Flag main loop to connect
      advertisedDevice.getScan()->stop();                   // Stop scanning (found target)
    }
  }
};

// === OLED DISPLAY FUNCTION (SHOWS MESSAGE AND STATUS BOXES) ===
void showOnDisplay(const String& msg) {
  u8g2.clearBuffer();                        // Clear the internal OLED buffer
  u8g2.setFont(u8g2_font_boutique_bitmap_7x7_te);           // Set a readable small font
  //u8g2.setFont(u8g2_font_5x7_tr);           // Set a good readable small font
  //u8g2.setFont(u8g2_font_4x6_tf);           // Set a better readable small font
  
  u8g2.drawStr(0, 9, "BLE Debug:");         // Add a fixed title at the top

  // -- Print up to 3 lines of the message, wrapping every 21 chars, 12px spacing
  int y = 28;                                // y-pos for first line
  int start = 0;                             // Start index for substring
  int len = msg.length();                    // Message length
  int lines = 0;                             // Line counter
  while (start < len && y < 52 && lines < 3) { // Only up to 3 lines to leave space for boxes
    String lineMsg = msg.substring(start, min(start + 30, len)); // was 21
    u8g2.drawStr(0, y, lineMsg.c_str());
    start += 30; // was 21
    y += 12;
    lines++;
  }

  // -- Draw the button status boxes at the bottom of the screen
  int boxX = 0;      // Initial x-position for first box
  int boxY = 62;     // y-position for all boxes (bottom of OLED)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonStates[i]) {
      u8g2.drawBox(boxX, boxY-8, 10, 10);    // Filled box if ON
    } else {
      u8g2.drawFrame(boxX, boxY-8, 10, 10);  // Empty box if OFF
    }
    boxX += 16;                              // Space out next box horizontally
  }
  u8g2.sendBuffer();                         // Push buffer to OLED for display
}

// === ARDUINO SETUP ===
void setup() {
  Serial.begin(115200);                      // Start serial for debug output

  // -- OLED INITIALIZATION
  u8g2.begin();                              // Initialize the OLED hardware and library
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 12, "Waiting for BLE...");
  u8g2.sendBuffer();                         // Show initial message

  // -- BUTTON PIN INITIALIZATION
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);   // Active LOW: button pulls pin to ground
  }

  // -- BLE SCAN INITIALIZATION
  BLEDevice::init("");                       // Initialize BLE stack (no client name needed)
  BLEScan* pBLEScan = BLEDevice::getScan();  // Create BLE scan object
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);             // More detailed advertising packets
  pBLEScan->start(30, false);                // Start scan (max 30 seconds, non-blocking)
}

// === ARDUINO MAIN LOOP ===
void loop() {
  // === BUTTON HANDLING: Polling, Debouncing, Command Sending ===
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]); // Read button state (LOW when pressed)
    // Detect any change in state for debounce logic
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();           // Reset debounce timer
    }
    // If state has been stable for debounceDelay, process it
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      // -- Button is pressed (LOW) and wasn't previously marked pressed
      if (reading == LOW && !buttonPressed[i]) {
        buttonPressed[i] = true;                   // Register as pressed
        // Send BLE command if connected and characteristic is available
        if (pCommandCharacteristic) {
          pCommandCharacteristic->writeValue(BUTTON_COMMANDS[i], strlen(BUTTON_COMMANDS[i]));
          String msg = "Sent: ";
          msg += BUTTON_COMMANDS[i];
          showOnDisplay(msg);                      // OLED feedback
          Serial.println(msg);                     // Serial feedback
        }
      }
      // -- Button is released (HIGH) after being pressed
      else if (reading == HIGH && buttonPressed[i]) {
        buttonPressed[i] = false;                  // Register as released
      }
    }
    lastButtonState[i] = reading;                  // Store for next loop
  }

  // === BLE CONNECTION LOGIC: Connect after scan, get characteristics, register notify ===
  if (doConnect && myDevice != nullptr) {
    doConnect = false;                          // Prevent repeated attempts
    Serial.println("Connecting to BLE server...");
    pClient = BLEDevice::createClient();        // Create BLE client object

    if (!pClient->connect(myDevice)) {          // Try to connect to server
      Serial.println("Failed to connect.");
      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "BLE Connect failed!");
      u8g2.sendBuffer();
      delay(2000);
      ESP.restart();                            // Restart ESP32 to retry connection
    }

    Serial.println("Connected as BLE client.");
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
      Serial.println("Failed to find BLE service.");
      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "Service not found!");
      u8g2.sendBuffer();
      delay(2000);
      ESP.restart();
    }

    // Retrieve pointers to the notification and write characteristics
    pStatusCharacteristic = pRemoteService->getCharacteristic(STATUS_CHAR_UUID);
    pCommandCharacteristic = pRemoteService->getCharacteristic(COMMAND_CHAR_UUID);

    // === REGISTER FOR NOTIFICATIONS: Update box state on relay "on"/"off" ===
    if (pStatusCharacteristic && pStatusCharacteristic->canNotify()) {
      pStatusCharacteristic->registerForNotify([](
        BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify
      ) {
        String statusMsg((char*)pData, length); // Convert notification to String

        // --- Update relay/command box states based on received message ---
        // For each command, check if received line contains "[command] on" or "[command] off"
        for (int i = 0; i < NUM_BUTTONS; i++) {
          String onMsg = String(BUTTON_COMMANDS[i]) + " on";
          String offMsg = String(BUTTON_COMMANDS[i]) + " off";
          if (statusMsg.indexOf(onMsg) >= 0) {
            buttonStates[i] = true;      // Fill box if "on"
          } else if (statusMsg.indexOf(offMsg) >= 0) {
            buttonStates[i] = false;     // Outline box if "off"
          }
        }

        String dispMsg = "Received: " + statusMsg;
        Serial.println(dispMsg);         // Print received to serial
        showOnDisplay(dispMsg);          // Display message + updated boxes
      });
    }
    showOnDisplay("BLE Connected!");     // Confirm connection on OLED
    Serial.println("Ready for BLE notifications.");
  }

  // === OPTIONAL: SEND PERIODIC "refresh" COMMAND EVERY 10 SECONDS ===
  static unsigned long lastCmd = 0;
  if (pCommandCharacteristic && millis() - lastCmd > 10000) {
    lastCmd = millis();
    String cmd = "refresh";
    pCommandCharacteristic->writeValue(cmd.c_str(), cmd.length());
    Serial.println("Sent command: " + cmd);
  }

  delay(10); // Short delay to avoid maxing CPU
}