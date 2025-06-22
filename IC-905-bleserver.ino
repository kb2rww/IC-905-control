// ==== IC-905 BLE SERVER with HW-246 Compass (QMC5883L) Support ====

// --- INCLUDE REQUIRED LIBRARIES ---
// ESP32 BLE (Bluetooth Low Energy) libraries.
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

// I2C and Compass sensor libraries for the HW-246 (QMC5883L) compass.
#include <Wire.h>
#include <QMC5883LCompass.h>  // Library: https://github.com/keepworking/DFRobot_QMC5883

// --- I2C PIN DEFINITIONS ---
// Change these to set your preferred ESP32 I2C pins for the compass.
#define I2C_SDA 21  // Example: GPIO18 for SDA
#define I2C_SCL 22  // Example: GPIO19 for SCL

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// These UUIDs uniquely identify your BLE service and characteristics.
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE SERVER VARIABLES ---
BLEServer* pServer = nullptr;               // Pointer to the BLE server instance.
BLEService* pService = nullptr;             // Pointer to the main BLE service.
BLECharacteristic* pStatusChar = nullptr;   // Characteristic for server-to-client notifications.
BLECharacteristic* pCommandChar = nullptr;  // Characteristic for client-to-server commands.

// --- CONNECTION STATE ---
bool deviceConnected = false;               // True if a BLE client is connected.

// --- COMMAND DEFINITIONS AND STATE TRACKING ---
#define NUM_COMMANDS 9                      // Total number of supported commands.
const char* COMMANDS[NUM_COMMANDS] = {      // List of command strings (must match client).
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
bool commandStates[NUM_COMMANDS] = {false}; // Tracks ON/OFF state for each command.
String lastSentStatus[NUM_COMMANDS];        // Stores last notified status per command.

// --- COMPASS (HEADING) STATE TRACKING ---
QMC5883LCompass compass;                    // The compass sensor object.
float lastSentHeading = -999.0;             // Last heading sent to client. Initialized to impossible value.

// --- HELPERS: COMMAND INDEX LOOKUP ---
// Returns the index for a given command string, or -1 if not found.
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;     // Found a matching command.
  }
  return -1;                                // Not found.
}

// --- GET CURRENT HEADING FROM COMPASS ---
// Returns heading in degrees (0-359), or -1 if sensor error.
float getCurrentHeading() {
  compass.read();                           // Request a new sensor reading.
  int heading = compass.getAzimuth();       // getAzimuth() should return 0-359.
  if (heading < 0 || heading > 359)         // Sanity-check result.
    return -1;                              // Sensor error or out of range.
  return (float)heading;                    // Return as float for compatibility.
}

// --- BLE SERVER CALLBACKS ---
// These handle BLE client connect/disconnect events.
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
    // Restart advertising so new clients can connect.
    BLEDevice::getAdvertising()->start();
  }
};

// --- BLE CHARACTERISTIC CALLBACKS ---
// Handles when the BLE client writes to the command characteristic.
class CommandCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    // Get the string value written by the client.
    String value = pCharacteristic->getValue();

    Serial.print("[BLE] Received command: ");
    Serial.println(value);

    // Special case: respond to "PING" with "PONG".
    if (value == "PING") {
      pStatusChar->setValue("PONG");
      pStatusChar->notify();
      Serial.println("[BLE] Sent PONG response");
      return;
    }

    // Check if value matches a known command/button.
    int idx = getCommandIndex(value);
    if (idx >= 0) {
      // Toggle the ON/OFF state for the command/button.
      commandStates[idx] = !commandStates[idx];

      // Format notification message: e.g., "relay 1 on" or "relay 1 off".
      String notifyMsg = String(COMMANDS[idx]) + (commandStates[idx] ? " on" : " off");

      // Only send notification if state actually changed.
      if (notifyMsg != lastSentStatus[idx]) {
        pStatusChar->setValue(notifyMsg.c_str());
        pStatusChar->notify();
        lastSentStatus[idx] = notifyMsg;
        Serial.print("[BLE] Status notified: ");
        Serial.println(notifyMsg);
      }
    }
    // For unknown commands, do nothing (could add error handling if desired).
  }
};

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);                 // Setup serial port for debug output.
  delay(200);                           // Allow time for serial port to stabilize.
  Serial.println("[BLE] Starting BLE Server...");

  // --- I2C BUS AND COMPASS SENSOR INITIALIZATION ---
  Wire.begin(I2C_SDA, I2C_SCL);         // Initialize I2C bus with custom SDA/SCL pins.
  compass.init();                       // Initialize the compass sensor.
  compass.setSmoothing(10, false);      // Smoothing for stable readings (optional).
  Serial.println("[BLE] HW-246 Compass (QMC5883L) initialized.");

  // --- BLE DEVICE INITIALIZATION ---
  BLEDevice::init("IC905_BLE_Server");  // Set BLE device name (as seen by clients).

  // --- CREATE BLE SERVER AND SERVICE ---
  pServer = BLEDevice::createServer();              // Create BLE server object.
  pServer->setCallbacks(new MyServerCallbacks());   // Set connect/disconnect callback.

  pService = pServer->createService(SERVICE_UUID);  // Create BLE service with defined UUID.

  // --- CREATE STATUS CHARACTERISTIC (server -> client notifications) ---
  pStatusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY              // Only notify (not writeable by client).
  );
  pStatusChar->addDescriptor(new BLE2902());        // Required for BLE notifications.
  pStatusChar->setValue("READY");                   // Set initial value.

  // --- CREATE COMMAND CHARACTERISTIC (client -> server writes) ---
  pCommandChar = pService->createCharacteristic(
    COMMAND_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE               // Client can write commands.
  );
  pCommandChar->setCallbacks(new CommandCharCallbacks()); // Set write handler.

  // --- START BLE SERVICE ---
  pService->start();

  // --- START BLE ADVERTISING ---
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);       // Advertise our service UUID.
  pAdvertising->setScanResponse(true);              // Enable scan response packets.
  pAdvertising->setMinPreferred(0x06);              // iOS compatibility tweak.
  pAdvertising->setMinPreferred(0x12);              // iOS compatibility tweak.
  BLEDevice::startAdvertising();                    // Start BLE advertising.

  Serial.println("[BLE] BLE server is now advertising!");
}

void loop() {
  // --- PERIODIC HEADING (COMPASS) NOTIFICATION ---
  static unsigned long lastHeadingNotify = 0;       // Timestamp of last compass notification.
  static float prevHeading = -999.0;                // For compass freeze detection.
  static unsigned long lastCompassChange = 0;       // Time of last heading change.

  // Send heading every 3 seconds if client is connected.
  if (deviceConnected && millis() - lastHeadingNotify > 3000) {
    lastHeadingNotify = millis();

    float heading = getCurrentHeading();            // Get latest heading from compass.
    Serial.print("[DEBUG] Raw heading: "); Serial.println(heading);

    // Only notify if heading is valid and has changed by at least 1 degree.
    if (heading >= 0 && abs(heading - lastSentHeading) >= 1.0) {
      String headingMsg = "heading: " + String((int)heading); // No decimal point.
      pStatusChar->setValue(headingMsg.c_str());
      pStatusChar->notify();
      lastSentHeading = heading;
      Serial.print("[BLE] Compass notified: ");
      Serial.println(headingMsg);
    }

    // --- COMPASS FREEZE DETECTION AND RECOVERY ---
    // If heading hasn't changed for 15s or is invalid, reinitialize the sensor.
    if (heading != prevHeading && heading >= 0) {
      lastCompassChange = millis();                 // Heading has changed; update timestamp.
      prevHeading = heading;
    } else if (millis() - lastCompassChange > 15000 || heading < 0) {
      // Sensor stuck or invalid, try to re-init.
      Serial.println("[ERROR] Compass freeze detected or invalid reading. Reinitializing sensor...");
      compass.init();
      compass.setSmoothing(10, false);
      lastCompassChange = millis();                 // Reset freeze timer.
    }
  }

  // --- STATE CHANGE NOTIFICATIONS FOR EACH COMMAND ---
  for (int i = 0; i < NUM_COMMANDS; i++) {
    String currentStatus = String(COMMANDS[i]) + (commandStates[i] ? " on" : " off");
    // Only send notification if state changed since last notification.
    if (currentStatus != lastSentStatus[i]) {
      pStatusChar->setValue(currentStatus.c_str());
      pStatusChar->notify();
      lastSentStatus[i] = currentStatus;
      Serial.print("[BLE] State change notified: ");
      Serial.println(currentStatus);
    }
  }

  delay(20); // Short delay to reduce CPU usage (adjust as needed).
}
