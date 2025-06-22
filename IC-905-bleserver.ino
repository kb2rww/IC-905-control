// ==== IC-905 BLE SERVER with HW-246 Compass (QMC5883L) Support ====

// --- INCLUDE REQUIRED LIBRARIES ---
// BLE libraries for ESP32
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

// I2C and Compass sensor libraries for HW-246 (QMC5883L)
#include <Wire.h>
#include <QMC5883LCompass.h>  // Install: https://github.com/keepworking/DFRobot_QMC5883

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// These MUST match your client code!
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE Server Variables ---
BLEServer* pServer = nullptr;           // BLE server instance
BLEService* pService = nullptr;         // Main BLE service
BLECharacteristic* pStatusChar = nullptr;   // Characteristic for server->client notifications
BLECharacteristic* pCommandChar = nullptr;  // Characteristic for client->server writes

// --- Connection State Variable ---
bool deviceConnected = false;           // True = client connected, false = not connected

// --- COMMANDS AND STATE TRACKING ---
// List of supported commands/buttons (MUST match client exactly)
#define NUM_COMMANDS 9
const char* COMMANDS[NUM_COMMANDS] = {
  "control power",    // 0
  "motor left",       // 1
  "motor right",      // 2
  "motor stop",       // 3
  "relay 1",          // 4
  "relay 2",          // 5
  "relay 3",          // 6
  "relay 4",          // 7
  "relay 5"           // 8
};
bool commandStates[NUM_COMMANDS] = {false};      // ON/OFF state for each command
String lastSentStatus[NUM_COMMANDS];             // Last notified status string per command

// --- HEADING (COMPASS) STATE TRACKING ---
QMC5883LCompass compass;         // HW-246 (QMC5883L) compass object
float lastSentHeading = -999.0;  // Last heading sent to client (init to impossible value)

// --- HELPER FUNCTION: Find Command Index ---
// Returns the index for the given command string; -1 if not found
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;
  }
  return -1;
}

// --- FUNCTION: Get Current Heading from Compass Sensor ---
// Returns heading in degrees (0-359), or -1 if sensor error
float getCurrentHeading() {
  compass.read();  // Update sensor data from hardware
  int heading = compass.getAzimuth(); // getAzimuth() returns 0-359 degrees
  if (heading < 0 || heading > 359) return -1; // Out of range? (shouldn't happen)
  return (float)heading;
}

// --- BLE SERVER CALLBACK CLASS ---
// Handles client connect/disconnect events
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

      // Only send if state actually changed
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
  delay(200); // Let serial stabilize
  Serial.println("[BLE] Starting BLE Server...");
  
  // --- COMPASS SENSOR INIT (HW-246 / QMC5883L) ---
  Wire.begin();           // Initialize I2C (SDA/SCL: 21/22 on ESP32 by default)
  compass.init();         // Initialize compass sensor
  compass.setSmoothing(10, false); // Smoothing for stable readings (optional)
  Serial.println("[BLE] HW-246 Compass (QMC5883L) initialized.");

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
  if (deviceConnected && millis() - lastHeadingNotify > 3000) { // Every 3 seconds
    lastHeadingNotify = millis();

    float heading = getCurrentHeading(); // Get latest heading from compass
    if (heading >= 0 && abs(heading - lastSentHeading) >= 1.0) { // Only send if changed by 1 degree or more
      String headingMsg = "heading: " + String(heading, 1); // 1 decimal place
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
