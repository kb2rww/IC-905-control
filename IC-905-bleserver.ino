// ==== IC-905 BLE SERVER with HW-246 (QMC5883L) COMPASS Support ====
// ---- EXTREMELY GRANULAR COMMENTARY VERSION ----

// --- INCLUDE REQUIRED LIBRARIES ---
#include <BLEDevice.h>         // Core BLE functions
#include <BLEServer.h>         // BLE server functionality
#include <BLEUtils.h>          // BLE utility functions (UUIDs, etc.)
#include <BLE2902.h>           // BLE descriptor (for notifications)
#include <Arduino.h>           // Arduino core
#include <QMC5883LCompass.h>   // QMC5883L compass library for HW-246

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

// --- COMPASS OBJECT (QMC5883L/HW-246) ---
QMC5883LCompass compass; // Create compass object

// --- HELPER FUNCTION: Find Command Index ---
// Returns the index for the given command string; -1 if not found
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;
  }
  return -1;
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
      // Send updated state as BLE notification to client
      pStatusChar->setValue(notifyMsg.c_str());
      pStatusChar->notify();
      Serial.print("[BLE] Status notified: ");
      Serial.println(notifyMsg);
    }
    // For unknown commands, no action taken (could add error handling here if desired)
  }
};

// --- READ & FORMAT COMPASS HEADING ---
// Returns heading as a string "xxx.x" degrees, always in range 0.0–359.9
String getCompassHeadingString() {
  compass.read(); // Update sensor values

  // Get the azimuth (heading) in degrees, as a float
  float heading = compass.getAzimuth();

  // Normalize: ensure heading is positive and < 360.0
  if (heading < 0.0) heading += 360.0;
  heading = fmod(heading, 360.0);

  // Format float to string with one decimal place (e.g., "273.8")
  char headingStr[8];
  dtostrf(heading, 5, 1, headingStr); // width=5, precision=1

  // Remove leading spaces (dtostrf pads by default)
  String cleanHeading = String(headingStr);
  cleanHeading.trim();
  return cleanHeading;
}

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE Server...");

  // --- INIT COMPASS SENSOR (HW-246/QMC5883L) ---
  compass.init(); // Initialize QMC5883L hardware

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
  // --- PERIODIC STATUS NOTIFICATION (every 3 seconds if a client is connected) ---
  static unsigned long lastNotify = 0;
  if (deviceConnected && millis() - lastNotify > 3000) {
    lastNotify = millis();

    // --- GET FORMATTED COMPASS HEADING (0.0–359.9° as string) ---
    String headingStr = getCompassHeadingString();

    // --- BUILD STATUS STRING ---
    // Format: "heading: 273.8; relay 1 on; relay 2 off; ..."
    String msg = "heading: ";
    msg += headingStr;

    // Append relay/button states
    for (int i = 0; i < NUM_COMMANDS; i++) {
      msg += "; ";
      msg += String(COMMANDS[i]) + (commandStates[i] ? " on" : " off");
    }

    // --- SEND STATUS AS BLE NOTIFICATION TO CLIENT ---
    pStatusChar->setValue(msg.c_str());
    pStatusChar->notify();
    Serial.print("[BLE] Status notified: ");
    Serial.println(msg);
  }

  // --- SMALL DELAY TO REDUCE CPU USAGE ---
  delay(100);
}

/*
  - This code reads the HW-246 (QMC5883L) compass every 3 seconds, formats the heading as "0.0–359.9" float, and sends it to the client.
  - Requires the QMC5883LCompass library. Wire up SCL/SDA for I2C.
  - The Nextion display should parse and show the heading as a float.
  - Relay/button status messages are unchanged.
  - All logic blocks are heavily commented for clarity.
*/
