// ==== IC-905 BLE Client for Heading Display, with Ultra-Granular Comments ====

// --- INCLUDE REQUIRED LIBRARIES ---
// U8g2 for OLED, BLE stack for ESP32, SPI for OLED display
#include <U8g2lib.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- OLED SPI PIN DEFINITIONS ---
// Make sure these match your hardware wiring!
#define OLED_CS    5                // Chip Select for SPI OLED
#define OLED_DC    17               // Data/Command select for SPI OLED
#define OLED_RESET 16               // Reset signal for SPI OLED

// --- CREATE OLED DISPLAY OBJECT ---
// This object lets us draw text/graphics to the OLED screen
U8G2_SSD1309_128X64_NONAME2_F_4W_HW_SPI u8g2(
  U8G2_R0,                         // No screen rotation
  OLED_CS,                         // Chip Select pin
  OLED_DC,                         // Data/Command pin
  OLED_RESET                       // Reset pin
);

// --- BLE SERVICE AND CHARACTERISTIC UUIDS ---
// These must match the server's UUIDs exactly!
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0" // Main BLE service
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1" // Notification char (server->client)
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2" // Write char (client->server)

// --- BLE CLIENT STATE VARIABLES ---
// Used to keep track of BLE connection and characteristics
static boolean doConnect = false;                  // True when we find server during scan
static BLEAdvertisedDevice* myDevice = nullptr;    // Holds info on discovered BLE server
BLEClient* pClient = nullptr;                      // BLE Client connection object
BLERemoteCharacteristic* pStatusCharacteristic = nullptr;  // For notifications from server
BLERemoteCharacteristic* pCommandCharacteristic = nullptr; // For sending commands to server

// --- BUTTON GPIO ASSIGNMENTS (6 BUTTONS) AND THEIR BLE COMMANDS ---
// These should match your hardware wiring and server's COMMANDS array
#define BUTTON1_PIN 4
#define BUTTON2_PIN 14
#define BUTTON3_PIN 12
#define BUTTON4_PIN 32
#define BUTTON5_PIN 25
#define BUTTON6_PIN 27
//#define BUTTON7_PIN 26
//#define BUTTON8_PIN 10

const int BUTTON_PINS[] = {
  BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN, BUTTON5_PIN, BUTTON6_PIN
};
const char* BUTTON_COMMANDS[] = {
  "control power", // Button 1
  "motor left",    // Button 2
  "motor right",   // Button 3
  "relay 1",       // Button 4
  "relay 2",       // Button 5
  "relay 3"        // Button 6
  //"relay 4"        // Button 7
  //"relay 5"        // Button 8
};
const int NUM_BUTTONS = 6;

// --- BUTTON DEBOUNCE STATE VARIABLES ---
// For reliable button reading (debouncing)
unsigned long lastDebounceTime[NUM_BUTTONS] = {0, 0, 0, 0, 0, 0};
bool lastButtonState[NUM_BUTTONS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool buttonPressed[NUM_BUTTONS] = {false, false, false, false, false, false};
const unsigned long debounceDelay = 50; // debounce time in ms

// --- VISUAL STATUS FOR BUTTON BOXES (FILLED = "ON" from server, EMPTY = "OFF") ---
bool buttonStates[NUM_BUTTONS] = {false, false, false, false, false, false};

// --- STORAGE FOR TRUE HEADING DATA ---
float latestTrueHeading = -1.0;     // Last received true north heading (degrees)
unsigned long lastHeadingReceived = 0; // Timestamp for heading display timeout

// --- OLED DISPLAY FUNCTION ---
// Shows title, optional message, 6 status boxes, and heading.
// Call this after any status, heading, or button event.
void showOnDisplay(const String& msg) {
  u8g2.clearBuffer();                        // Clear the internal OLED buffer

  // --- Show a title at the top for context ---
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "IC-905 BLE Client");  // Always show at top

  // --- Show the main message (e.g., status, errors, etc.) ---
  //u8g2.setFont(u8g2_font_5x7_tr); // Smaller font for messages
  //int msgY = 22;
  //int start = 0;
  //int len = msg.length();
  //int lines = 0;
  // Print up to 2 lines of the message, wrapping every 28 chars
  //while (start < len && lines < 2) {
  //  String lineMsg = msg.substring(start, min(start + 28, len));
  //  u8g2.drawStr(0, msgY, lineMsg.c_str());
  //   start += 28;
  //  msgY += 10;
  //  lines++;
  //}

  // --- Show the latest true heading if recent (received in last 5s) ---
  if (millis() - lastHeadingReceived < 5000 && latestTrueHeading >= 0) {
    char headingBuf[32];
    snprintf(headingBuf, sizeof(headingBuf), "True North: %5.1f deg", latestTrueHeading);
    u8g2.setFont(u8g2_font_5x7_mr);
    u8g2.drawStr(0, 44, headingBuf);
  } else {
    // If no recent heading, print a placeholder
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 44, "Heading: ---");
  }

  // --- Draw the button status boxes at the bottom of the screen ---
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

// --- BLE SCAN CALLBACK HANDLER CLASS ---
// This class handles BLE advertisements during scanning and sets doConnect when our server is found
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Check if the advertised device offers the target service UUID
    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
      myDevice = new BLEAdvertisedDevice(advertisedDevice); // Copy device info
      doConnect = true;                                     // Signal main loop to connect
      advertisedDevice.getScan()->stop();                   // Stop scanning (found our device)
    }
  }
};

// --- BLE NOTIFICATION CALLBACK FUNCTION ---
// Handles incoming notifications from the server (status and heading updates)
void statusNotifyCB(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify
) {
  String statusMsg((char*)pData, length); // Convert notification data to String

  // --- Update relay/command button box states based on received status ---
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

  // --- If message is a heading, extract and store true north value ---
  if (statusMsg.startsWith("HEADING:")) {
    // Example: HEADING:115.5
    float trueHeading = statusMsg.substring(8).toFloat();
    latestTrueHeading = trueHeading;
    lastHeadingReceived = millis();
  }

  // --- Show all info on OLED (status + heading) ---
showOnDisplay(statusMsg);

  // --- For debugging: print to serial monitor as well ---
  Serial.println("[BLE Notify] " + statusMsg);
}

// --- ARDUINO SETUP ---
void setup() {
  Serial.begin(115200);                      // Start serial for debug output

  // --- OLED INITIALIZATION ---
  u8g2.begin();                              // Initialize the OLED hardware and library
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 12, "Waiting for BLE...");
  u8g2.sendBuffer();                         // Show initial message

  // --- BUTTON PIN INITIALIZATION ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);   // Active LOW: button pulls pin to ground
  }

  // --- BLE SCAN INITIALIZATION ---
  BLEDevice::init("");                       // Initialize BLE stack (no client name needed)
  BLEScan* pBLEScan = BLEDevice::getScan();  // Create BLE scan object
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);             // More detailed advertising packets
  pBLEScan->start(30, false);                // Start scan (max 30 seconds, non-blocking)
}

// --- ARDUINO MAIN LOOP ---
void loop() {
  // --- BUTTON HANDLING: Polling, Debouncing, Command Sending ---
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

  // --- BLE CONNECTION LOGIC: Connect after scan, get characteristics, register notify ---
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

    // --- REGISTER FOR NOTIFICATIONS: Handles all server->client messages including heading ---
    if (pStatusCharacteristic && pStatusCharacteristic->canNotify()) {
      pStatusCharacteristic->registerForNotify(statusNotifyCB);
    }
    showOnDisplay("BLE Connected!");     // Confirm connection on OLED
    Serial.println("Ready for BLE notifications.");
  }

  // --- OPTIONAL: Send periodic request for latest heading every 10 seconds ---
  static unsigned long lastHeadingRequest = 0;
  if (pCommandCharacteristic && millis() - lastHeadingRequest > 10000) {
    lastHeadingRequest = millis();
    String cmd = "READ_HEADING";
    pCommandCharacteristic->writeValue(cmd.c_str(), cmd.length());
    Serial.println("Sent command: " + cmd);
  }

  delay(10); // Short delay to avoid maxing CPU
}
