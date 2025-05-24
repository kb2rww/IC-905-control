// === IC905 BLE CLIENT with 8 BUTTONS/RELAYS and Granular Comments ===

// --- INCLUDE NECESSARY LIBRARIES ---
#include <U8g2lib.h>        // For OLED display rendering
#include <SPI.h>            // For SPI communication with OLED
#include <BLEDevice.h>      // ESP32 BLE stack main library
#include <BLEUtils.h>       // BLE utility functions
#include <BLEScan.h>        // BLE scanning (finding devices)
#include <BLEAdvertisedDevice.h> // Handling BLE advertisement data

// --- OLED SPI PIN DEFINITIONS ---
#define OLED_CS    5        // Chip Select for OLED (SPI)
#define OLED_DC    17       // Data/Command select for OLED (SPI)
#define OLED_RESET 16       // Reset signal for OLED (SPI)

// --- CREATE OLED DISPLAY OBJECT ---
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(
  U8G2_R0, OLED_CS, OLED_DC, OLED_RESET
);

// --- BLE SERVICE AND CHARACTERISTIC UUIDS (MUST MATCH SERVER) ---
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0" // Main BLE service
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1" // Notification char
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2" // Write char

// --- BLE CLIENT STATE VARIABLES ---
static boolean doConnect = false;                  // True: begin connection process
static BLEAdvertisedDevice* myDevice = nullptr;    // Found BLE device pointer
BLEClient* pClient = nullptr;                      // BLE client connection object
BLERemoteCharacteristic* pStatusCharacteristic = nullptr;  // For notifications from server
BLERemoteCharacteristic* pCommandCharacteristic = nullptr; // For sending commands to server

// --- BUTTON GPIO ASSIGNMENTS (8 BUTTONS/RELAYS) ---
// Update these pin numbers to match your wiring!
#define BUTTON1_PIN 4      // Button 1 (GPIO4)
#define BUTTON2_PIN 14     // Button 2 (GPIO14)
#define BUTTON3_PIN 12     // Button 3 (GPIO12)
#define BUTTON4_PIN 32     // Button 4 (GPIO32)
#define BUTTON5_PIN 25     // Button 5 (GPIO25)
#define BUTTON6_PIN 27     // Button 6 (GPIO27)
#define BUTTON7_PIN 33     // Button 7 (GPIO33) [Relay 4]
#define BUTTON8_PIN 26     // Button 8 (GPIO26) [Relay 5]

// Array of all button pins for easy looping
const int BUTTON_PINS[] = {
  BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN,
  BUTTON5_PIN, BUTTON6_PIN, BUTTON7_PIN, BUTTON8_PIN
};

// Array of commands for each button, must match server
const char* BUTTON_COMMANDS[] = {
  "control power", // Button 1 (GPIO4)
  "motor left",    // Button 2 (GPIO14)
  "motor right",   // Button 3 (GPIO12)
  "relay 1",       // Button 4 (GPIO32)
  "relay 2",       // Button 5 (GPIO25)
  "relay 3",       // Button 6 (GPIO27)
  "relay 4",       // Button 7 (GPIO33)
  "relay 5"        // Button 8 (GPIO26)
};
const int NUM_BUTTONS = 8; // Now using 8 buttons

// --- BUTTON DEBOUNCE STATE VARIABLES ---
// Track last debounce time for each button (millis)
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};
// Last stable state (HIGH = not pressed due to INPUT_PULLUP)
bool lastButtonState[NUM_BUTTONS] = {HIGH};
// Whether button is currently pressed (for edge detection)
bool buttonPressed[NUM_BUTTONS] = {false};
// Minimum ms for a stable press (debounce logic)
const unsigned long debounceDelay = 50;

// --- VISUAL STATUS FOR BOXES (FILLED WHEN "ON" FROM SERVER, EMPTY OTHERWISE) ---
bool buttonStates[NUM_BUTTONS] = {false}; // false=off, true=on

// --- BLE SCAN CALLBACK HANDLER ---
// This class is called for each BLE advertisement found during scan
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Only connect if this device advertises the correct service
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      myDevice = new BLEAdvertisedDevice(advertisedDevice); // Copy info
      doConnect = true;                                     // Flag to connect in main loop
      advertisedDevice.getScan()->stop();                   // Stop scanning
    }
  }
};

// --- OLED DISPLAY FUNCTION ---
// Shows message + status boxes for each relay/button
void showOnDisplay(const String& msg) {
  u8g2.clearBuffer();                        // Clear OLED drawing buffer
  u8g2.setFont(u8g2_font_boutique_bitmap_7x7_te); // Small readable font

  u8g2.drawStr(0, 9, "BLE Debug:");          // Fixed title at top

  // --- Print up to 3 lines of the message, wrapping every 30 chars, 12px spacing
  int y = 28;                                // y-pos for first line of text
  int start = 0;                             // Start index for substring
  int len = msg.length();                    // Message length
  int lines = 0;                             // Line counter
  while (start < len && y < 52 && lines < 3) {
    String lineMsg = msg.substring(start, min(start + 30, len)); // Up to 30 chars per line
    u8g2.drawStr(0, y, lineMsg.c_str());
    start += 30;
    y += 12;
    lines++;
  }

  // --- Draw the button/relay status boxes at the bottom of the screen ---
  int boxX = 0;      // Start x for first box
  int boxY = 62;     // y-position for boxes (bottom of OLED)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (buttonStates[i]) {
      u8g2.drawBox(boxX, boxY-8, 10, 10);    // Filled box if ON
    } else {
      u8g2.drawFrame(boxX, boxY-8, 10, 10);  // Empty box if OFF
    }
    boxX += 14; // Space out boxes (tighter for 8 boxes)
  }
  u8g2.sendBuffer();                         // Push buffer to OLED for display
}

// --- ARDUINO SETUP ---
void setup() {
  Serial.begin(115200);                      // Serial for debug output

  // --- OLED INITIALIZATION ---
  u8g2.begin();                              // Initialize OLED
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 12, "Waiting for BLE...");
  u8g2.sendBuffer();                         // Show initial message

  // --- BUTTON PIN INITIALIZATION ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);   // Active LOW: button pulls pin to ground
  }

  // --- BLE SCAN INITIALIZATION ---
  BLEDevice::init("");                       // Initialize BLE stack (no device name needed)
  BLEScan* pBLEScan = BLEDevice::getScan();  // Create BLE scan object
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);             // More detailed advertising
  pBLEScan->start(30, false);                // Start scan (max 30s, non-blocking)
}

// --- ARDUINO MAIN LOOP ---
void loop() {
  // --- BUTTON POLLING, DEBOUNCE, AND BLE COMMAND SEND ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]); // Read button (LOW when pressed)
    // Debounce: detect change in input
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis(); // Reset debounce timer
    }
    // Debounce: if stable for debounceDelay, process
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      // Button pressed (LOW) and wasn't previously pressed
      if (reading == LOW && !buttonPressed[i]) {
        buttonPressed[i] = true;                   // Register as pressed
        // Send BLE command if connected and char available
        if (pCommandCharacteristic) {
          pCommandCharacteristic->writeValue(BUTTON_COMMANDS[i], strlen(BUTTON_COMMANDS[i]));
          String msg = "Sent: ";
          msg += BUTTON_COMMANDS[i];
          showOnDisplay(msg);                      // OLED feedback
          Serial.println(msg);                     // Serial feedback
        }
      }
      // Button released (HIGH) after being pressed
      else if (reading == HIGH && buttonPressed[i]) {
        buttonPressed[i] = false;                  // Register as released
      }
    }
    lastButtonState[i] = reading;                  // Store for next loop
  }

  // --- BLE CONNECTION LOGIC: Connect, get characteristics, register notify ---
  if (doConnect && myDevice != nullptr) {
    doConnect = false;                          // Prevent repeated attempts
    Serial.println("Connecting to BLE server...");
    pClient = BLEDevice::createClient();        // Create BLE client

    if (!pClient->connect(myDevice)) {          // Try to connect to server
      Serial.println("Failed to connect.");
      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "BLE Connect failed!");
      u8g2.sendBuffer();
      delay(2000);
      ESP.restart();                            // Restart ESP32 to retry
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

    // Retrieve notification and write characteristics
    pStatusCharacteristic = pRemoteService->getCharacteristic(STATUS_CHAR_UUID);
    pCommandCharacteristic = pRemoteService->getCharacteristic(COMMAND_CHAR_UUID);

    // --- REGISTER FOR NOTIFICATIONS: Update box state on "on"/"off" messages ---
    if (pStatusCharacteristic && pStatusCharacteristic->canNotify()) {
      pStatusCharacteristic->registerForNotify([](
        BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify
      ) {
        String statusMsg((char*)pData, length); // Notification to String

        // For each command, check for "[command] on" or "[command] off"
        for (int i = 0; i < NUM_BUTTONS; i++) {
          String onMsg = String(BUTTON_COMMANDS[i]) + " on";
          String offMsg = String(BUTTON_COMMANDS[i]) + " off";
          if (statusMsg.indexOf(onMsg) >= 0) {
            buttonStates[i] = true;      // Box filled if "on"
          } else if (statusMsg.indexOf(offMsg) >= 0) {
            buttonStates[i] = false;     // Box outlined if "off"
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

  // --- OPTIONAL: SEND PERIODIC "refresh" COMMAND EVERY 10 SECONDS ---
  static unsigned long lastCmd = 0;
  if (pCommandCharacteristic && millis() - lastCmd > 10000) {
    lastCmd = millis();
    String cmd = "refresh";
    pCommandCharacteristic->writeValue(cmd.c_str(), cmd.length());
    Serial.println("Sent command: " + cmd);
  }

  delay(10); // Short delay to avoid maxing CPU
}
