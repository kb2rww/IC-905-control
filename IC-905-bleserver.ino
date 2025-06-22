// ==== IC-905 BLE SERVER with HW-246 Compass (QMC5883L) Support ====

// --- INCLUDE REQUIRED LIBRARIES ---
// BLE libraries for ESP32 (Bluetooth Low Energy server functionality)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

// I2C and Compass sensor libraries for HW-246 (QMC5883L)
#include <Wire.h>
#include <QMC5883LCompass.h>  // Compass sensor: https://github.com/keepworking/DFRobot_QMC5883

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// These UUIDs identify your BLE service and characteristics. They must match your client/app.
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE Server Variables ---
BLEServer* pServer = nullptr;           // BLE server instance pointer
BLEService* pService = nullptr;         // Main BLE service pointer
BLECharacteristic* pStatusChar = nullptr;   // Characteristic for server-to-client notifications
BLECharacteristic* pCommandChar = nullptr;  // Characteristic for client-to-server commands

// --- Connection State Variable ---
bool deviceConnected = false;           // Tracks whether a BLE client is connected

// --- COMMANDS AND STATE TRACKING ---
// List of the possible control commands/buttons (must match your BLE client interface)
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
// State of each command (true = ON, false = OFF)
bool commandStates[NUM_COMMANDS] = {false};
// Last sent status message for each command (to prevent redundant notifications)
String lastSentStatus[NUM_COMMANDS];

// --- HEADING (COMPASS) STATE TRACKING ---
QMC5883LCompass compass;         // The compass sensor object
float lastSentHeading = -999.0;  // Last heading value sent to the client (impossible initial value)

// --- HELPER FUNCTION: Find Command Index ---
// Returns the array index for a command string, or -1 if not found
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;
  }
  return -1;
}

// --- FUNCTION: Get Current Heading from Compass Sensor ---
// Reads compass sensor, returns heading in degrees (0-359), or -1 if sensor error
float getCurrentHeading() {
  compass.read();                    // Update sensor data from hardware
  int heading = compass.getAzimuth();// getAzimuth() returns 0-359 degrees
  if (heading < 0 || heading > 359)  // Out-of-range values should never happen
    return -1;
  return (float)heading;             // Return as float for compatibility
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
    // Restart advertising when client disconnects so new clients can connect
    BLEDevice::getAdvertising()->start();
  }
};

// --- BLE CHARACTERISTIC CALLBACK CLASS ---
// Handles incoming writes to the command characteristic from the client
class CommandCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    // Read the string written by the client
    String value = pCharacteristic->getValue();

    Serial.print("[BLE] Received command: ");
    Serial.println(value);

    // --- Special command: PING (reply with PONG) ---
    if (value == "PING") {
      pStatusChar->setValue("PONG");
      pStatusChar->notify();
      Serial.println("[BLE] Sent PONG response");
      return;
    }

    // --- If value matches a known command/button, toggle its state ---
    int idx = getCommandIndex(value);
    if (idx >= 0) {
      // Toggle ON/OFF state for the command/button
      commandStates[idx] = !commandStates[idx];

      // Format notification: e.g., "relay 1 on" or "relay 1 off"
      String notifyMsg = String(COMMANDS[idx]) + (commandStates[idx] ? " on" : " off");

      // Only send if state actually changed (avoid duplicate notifications)
      if (notifyMsg != lastSentStatus[idx]) {
        pStatusChar->setValue(notifyMsg.c_str());
        pStatusChar->notify();
        lastSentStatus[idx] = notifyMsg;
        Serial.print("[BLE] Status notified: ");
        Serial.println(notifyMsg);
      }
    }
    // For unknown commands, no action taken (could add error handling here)
  }
};

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);       // Start serial for debug output
  delay(200);                 // Let serial port stabilize
  Serial.println("[BLE] Starting BLE Server...");
  
  // --- COMPASS SENSOR INITIALIZATION ---
  Wire.begin();               // Initialize I2C bus (default ESP32 pins: SDA=21, SCL=22)
  compass.init();             // Initialize compass (QMC5883L)
  compass.setSmoothing(10, false); // Smoothing for more stable readings (optional)
  Serial.println("[BLE] HW-246 Compass (QMC5883L) initialized.");

  // --- BLE STACK & DEVICE SETUP ---
  BLEDevice::init("IC905_BLE_Server"); // Set BLE device name (shows in scans)

  // --- CREATE BLE SERVER AND SERVICE ---
  pServer = BLEDevice::createServer();             // Create BLE server object
  pServer->setCallbacks(new MyServerCallbacks());  // Set connect/disconnect callback

  pService = pServer->createService(SERVICE_UUID); // Create BLE service with custom UUID

  // --- CREATE STATUS CHARACTERISTIC (notifications: server -> client) ---
  pStatusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY             // Only notify (can't be written by client)
  );
  pStatusChar->addDescriptor(new BLE2902());       // Required for notifications
  pStatusChar->setValue("READY");                  // Set initial status value

  // --- CREATE COMMAND CHARACTERISTIC (writes: client -> server) ---
  pCommandChar = pService->createCharacteristic(
    COMMAND_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE              // Client can write to this characteristic
  );
  pCommandChar->setCallbacks(new CommandCharCallbacks()); // Set write handler

  // --- START BLE SERVICE ---
  pService->start();

  // --- START BLE ADVERTISING (so clients can find us) ---
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise our main service UUID
  pAdvertising->setScanResponse(true);        // Enable scan response packets for faster discovery
  pAdvertising->setMinPreferred(0x06);        // iOS compatibility tweak
  pAdvertising->setMinPreferred(0x12);        // iOS compatibility tweak
  BLEDevice::startAdvertising();              // Begin advertising

  Serial.println("[BLE] BLE server is now advertising!");
}

void loop() {
  static unsigned long lastHeadingNotify = 0;
  static float prevHeading = -999.0;
  static unsigned long lastCompassChange = 0;

  if (deviceConnected && millis() - lastHeadingNotify > 3000) { // Every 3 seconds
    lastHeadingNotify = millis();

    float heading = getCurrentHeading(); // Get latest heading from compass
    Serial.print("[DEBUG] Raw heading: "); Serial.println(heading);

    if (heading >= 0 && abs(heading - lastSentHeading) >= 1.0) {
      String headingMsg = "heading: " + String((int)heading);
      pStatusChar->setValue(headingMsg.c_str());
      pStatusChar->notify();
      lastSentHeading = heading;
      Serial.print("[BLE] Compass notified: ");
      Serial.println(headingMsg);
    }

    // Detect heading freeze (no change for 15s or sensor error)
    if (heading != prevHeading && heading >= 0) {
      lastCompassChange = millis();
      prevHeading = heading;
    } else if (millis() - lastCompassChange > 15000 || heading < 0) {
      // Sensor stuck or invalid, try to re-init
      Serial.println("[ERROR] Compass freeze detected or invalid reading. Reinitializing sensor...");
      compass.init();
      compass.setSmoothing(10, false);
      lastCompassChange = millis();
    }
  }

  // --- STATE CHANGE NOTIFICATIONS FOR EACH COMMAND ---
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

  delay(20);
}
