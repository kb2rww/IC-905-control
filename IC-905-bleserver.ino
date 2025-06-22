// ==== IC-905 BLE SERVER with HW-246 (QMC5883L) COMPASS Support ====
// ---- EXTREMELY GRANULAR COMMENTARY VERSION ----

// ==== IC-905 BLE SERVER with 8 BUTTONS/RELAYS and Compass Status ====

// --- INCLUDE REQUIRED LIBRARIES ---
#include <BLEDevice.h>         // Core BLE functions
#include <BLEServer.h>         // BLE server functionality
#include <BLEUtils.h>          // BLE utility functions (UUIDs, etc.)
#include <BLE2902.h>           // BLE descriptor (for notifications)
#include <Arduino.h>           // Arduino core

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// (MUST match the client code)
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE Server Pointers ---
BLEServer* pServer = nullptr;           // BLE server instance
BLEService* pService = nullptr;         // Main BLE service
BLECharacteristic* pStatusChar = nullptr;   // Characteristic for server->client notifications
BLECharacteristic* pCommandChar = nullptr;  // Characteristic for client->server writes

// --- Connection State Variable ---
bool deviceConnected = false;           // True = client connected, false = not connected

// --- COMMANDS AND STATE TRACKING ---
// List of supported commands/buttons (must match client exactly)
#define NUM_COMMANDS 9
const char* COMMANDS[NUM_COMMANDS] = {
  "control power",    // Button 1
  "motor left",       // Button 2
  "motor right",      // Button 3
  "motor stop",       // Button 4
  "relay 1",          // Button 5
  "relay 2",          // Button 6
  "relay 3",          // Button 7
  "relay 4",          // Button 8
  "relay 5"           // Button 9
};
// Array to track ON/OFF state for each command: false = OFF, true = ON
bool commandStates[NUM_COMMANDS] = {false};

// --- TRACK LAST SENT STATUS FOR STATE CHANGE DETECTION ---
String lastSentStatus[NUM_COMMANDS]; // Stores last notified state for each command, e.g., "relay 1 on"
float lastSentHeading = -999.0;      // Stores last sent heading value (initialized to impossible value)

// --- HELPER FUNCTION: Find Command Index ---
// Returns the index for the given command string; -1 if not found
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;
  }
  return -1;
}

// --- MOCKUP: GET CURRENT HEADING ---
// Replace this with your real compass/heading reading!
float getCurrentHeading() {
  // For demonstration, we'll just increment heading value every call
  static float heading = 0.0;
  heading += 1.0;
  if (heading > 359.9) heading = 0.0;
  return heading;
}

// --- BLE SERVER CALLBACK CLASS ---
// Handles client connect and disconnect events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
    // Restart advertising to allow new connections
    BLEDevice::getAdvertising()->start();
  }
};

// --- BLE CHARACTERISTIC CALLBACK CLASS ---
// Handles when the client writes to the command characteristic
class CommandCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue(); // Get string written by client

    Serial.print("[BLE] Received command: ");
    Serial.println(value);

    // --- Special command: PING (reply with PONG) ---
    if (value == "PING") {
      pStatusChar->setValue("PONG");
      pStatusChar->notify();
      Serial.println("[BLE] Sent PONG response");
      return;
    }

    // --- Check if value matches a known command/button ---
    int idx = getCommandIndex(value);
    if (idx >= 0) {
      // Toggle the ON/OFF state for the command/button
      commandStates[idx] = !commandStates[idx];

      // Format notification: e.g., "relay 1 on" or "relay 1 off"
      String notifyMsg = String(COMMANDS[idx]) + (commandStates[idx] ? " on" : " off");

      // Only send if state actually changed (protects against double-toggles etc)
      if (notifyMsg != lastSentStatus[idx]) {
        pStatusChar->setValue(notifyMsg.c_str());
        pStatusChar->notify();
        lastSentStatus[idx] = notifyMsg;
        Serial.print("[BLE] Status notified: ");
        Serial.println(notifyMsg);
      }
    }
    // For unknown commands, no action taken (could add error handling here if desired)
  }
};

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE Server...");

  // --- BLE INITIALIZATION ---
  BLEDevice::init("IC905_BLE_Server");             // Set BLE device name

  // --- CREATE BLE SERVER AND SERVICE ---
  pServer = BLEDevice::createServer();             // Create BLE server object
  pServer->setCallbacks(new MyServerCallbacks());  // Set connect/disconnect callbacks

  pService = pServer->createService(SERVICE_UUID); // Create main BLE service

  // --- CREATE STATUS CHARACTERISTIC (notifications: server -> client) ---
  pStatusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY               // Notify property only
  );
  pStatusChar->addDescriptor(new BLE2902());        // Required for notifications
  pStatusChar->setValue("READY");                   // Set initial status value

  // --- CREATE COMMAND CHARACTERISTIC (writes: client -> server) ---
  pCommandChar = pService->createCharacteristic(
    COMMAND_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE                // Write property only
  );
  pCommandChar->setCallbacks(new CommandCharCallbacks()); // Set write handler

  // --- START BLE SERVICE ---
  pService->start();

  // --- START ADVERTISING (so clients can find us) ---
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise our main service UUID
  pAdvertising->setScanResponse(true);        // Enable scan response packets
  pAdvertising->setMinPreferred(0x06);        // iOS compatibility tweak
  pAdvertising->setMinPreferred(0x12);        // iOS compatibility tweak
  BLEDevice::startAdvertising();              // Begin advertising

  Serial.println("[BLE] BLE server is now advertising!");
}

void loop() {
  // --- PERIODIC HEADING NOTIFICATION ---
  static unsigned long lastHeadingNotify = 0;
  if (deviceConnected && millis() - lastHeadingNotify > 3000) { // Send heading every 3 seconds
    lastHeadingNotify = millis();

    float heading = getCurrentHeading(); // Get latest heading (replace with real reading)

    // Only send if heading changed by at least 1 degree (to avoid flooding)
    if (abs(heading - lastSentHeading) >= 1.0) {
      String headingMsg = "heading: " + String(heading, 1);
      pStatusChar->setValue(headingMsg.c_str());
      pStatusChar->notify();
      lastSentHeading = heading;
      Serial.print("[BLE] Compass notified: ");
      Serial.println(headingMsg);
    }
  }

  // --- STATE CHANGE NOTIFICATIONS FOR EACH COMMAND ---
  // Only send a notification if a state actually changed since last notify
  for (int i = 0; i < NUM_COMMANDS; i++) {
    String currentStatus = String(COMMANDS[i]) + (commandStates[i] ? " on" : " off");
    if (currentStatus != lastSentStatus[i]) {
      pStatusChar->setValue(currentStatus.c_str());
      pStatusChar->notify();
      lastSentStatus[i] = currentStatus;
      Serial.print("[BLE] State change notified: ");
      Serial.println(currentStatus);
    }
  }

  delay(20); // Reduce CPU usage, adjust as necessary
}
