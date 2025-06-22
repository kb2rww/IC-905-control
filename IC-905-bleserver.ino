// ==== IC-905 BLE SERVER with HW-246/QMC5883L Compass Support ====
// ---- BLE + BUTTONS/RELAYS + HEADING NOTIFICATIONS + GRANULAR COMMENTS ----

// --- INCLUDE REQUIRED LIBRARIES ---
#include <BLEDevice.h>         // Core Bluetooth Low Energy functions
#include <BLEServer.h>         // BLE server functionality helpers
#include <BLEUtils.h>          // BLE utility functions (UUIDs, etc.)
#include <BLE2902.h>           // BLE descriptor (for notifications)
#include <Arduino.h>           // Arduino core functions (digitalRead, etc.)
#include <QMC5883LCompass.h>   // QMC5883L compass (HW-246) library

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// (These UUIDs must match the client side for proper communication)
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE Server Pointers (global for access in callbacks) ---
BLEServer* pServer = nullptr;                // Pointer to BLE server instance
BLEService* pService = nullptr;              // Pointer to BLE service instance
BLECharacteristic* pStatusChar = nullptr;    // Pointer to characteristic for notifications (server->client)
BLECharacteristic* pCommandChar = nullptr;   // Pointer to characteristic for receiving commands (client->server)

// --- BLE Connection State Variable ---
bool deviceConnected = false;                // Tracks if a BLE client is connected

// --- COMMANDS AND STATE TRACKING ---
// Array of supported command strings (must match client exactly)
#define NUM_COMMANDS 9
const char* COMMANDS[NUM_COMMANDS] = {
  "control power",    // Button 1
  "motor left",       // Button 2
  "motor right",      // Button 3
  "motor stop",       // Button 33 (NEW)
  "relay 1",          // Button 4
  "relay 2",          // Button 5
  "relay 3",          // Button 6
  "relay 4",          // Button 7 (NEW)
  "relay 5"           // Button 8 (NEW)
};
// Array to track ON/OFF state for each command/button
bool commandStates[NUM_COMMANDS] = {false};  // false = OFF, true = ON

// --- HW-246 (QMC5883L) COMPASS SUPPORT ---
// Create global QMC5883LCompass object
QMC5883LCompass compass;

// --- SERIAL2 FOR NEXTION DISPLAY SUPPORT ---
// Uncomment and configure if using Serial2 for Nextion
//#define NEXTION_BAUDRATE 9600
//#define NEXTION_SERIAL Serial2

// --- HELPER FUNCTION: Find Index of Command String ---
// Returns index of a command, or -1 if not found
int getCommandIndex(const String& value) {
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (value == COMMANDS[i]) return i;
  }
  return -1;
}

// --- BLE SERVER CALLBACK CLASS ---
// Handles BLE client connect/disconnect events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
    // Immediately restart BLE advertising
    BLEDevice::getAdvertising()->start();
  }
};

// --- BLE CHARACTERISTIC CALLBACK CLASS ---
// Handles when the client writes a command to the server
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

    // --- Special command: get heading (reply with current compass heading) ---
    if (value == "get heading") {
      compass.read(); // Read from compass
      int heading = compass.getAzimuth(); // Get heading in degrees (0-359)
      String notifyMsg = "heading: " + String(heading);
      pStatusChar->setValue(notifyMsg.c_str());
      pStatusChar->notify();
      Serial.print("[BLE] Sent heading: ");
      Serial.println(notifyMsg);

      // --- Optionally send to Nextion display ---
      //NEXTION_SERIAL.print("heading=");
      //NEXTION_SERIAL.println(heading);
      return;
    }

    // --- Check if value matches a known command/button ---
    int idx = getCommandIndex(value);
    if (idx >= 0) {
      // Toggle ON/OFF state of button/relay
      commandStates[idx] = !commandStates[idx];
      // Format notification string, e.g. "relay 1 on" or "relay 1 off"
      String notifyMsg = String(COMMANDS[idx]) + (commandStates[idx] ? " on" : " off");
      // Notify client of new state
      pStatusChar->setValue(notifyMsg.c_str());
      pStatusChar->notify();
      Serial.print("[BLE] Status notified: ");
      Serial.println(notifyMsg);
    }
    // For unknown commands, do nothing (could add error handling here)
  }
};

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE Server...");

  // --- OPTIONAL: INIT SERIAL2 FOR NEXTION DISPLAY ---
  //NEXTION_SERIAL.begin(NEXTION_BAUDRATE);
  //Serial.println("[Nextion] Serial2 initialized");

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

  // --- HW-246/QMC5883L COMPASS INITIALIZATION ---
  compass.init(); // Initialize compass sensor (uses I2C)
  // Optionally set calibration here if needed
  Serial.println("[HW-246] QMC5883L compass initialized");
}

void loop() {
  // --- PERIODIC STATUS + HEADING NOTIFICATION ---
  static unsigned long lastNotify = 0; // Time of last notification (ms)
  const unsigned long notifyInterval = 3000; // Notification interval (ms)

  if (deviceConnected && millis() - lastNotify > notifyInterval) {
    lastNotify = millis();

    // --- Get current heading from HW-246 compass ---
    compass.read(); // Read new sensor data from QMC5883L
    int heading = compass.getAzimuth(); // Get current heading (0-359 degrees)

    // --- Build heading message string ---
    String headingMsg = "heading: " + String(heading);

    // --- Build a status string for all buttons/relays ---
    String buttonMsg;
    for (int i = 0; i < NUM_COMMANDS; i++) {
      buttonMsg += String(COMMANDS[i]) + (commandStates[i] ? " on" : " off");
      if (i != NUM_COMMANDS - 1) buttonMsg += "; ";
    }

    // --- Combine heading and button status for notification ---
    String fullMsg = headingMsg + "; " + buttonMsg;

    // --- Send status + heading as BLE notification to client ---
    pStatusChar->setValue(fullMsg.c_str());
    pStatusChar->notify();
    Serial.print("[BLE] Status notified: ");
    Serial.println(fullMsg);

    // --- Optionally send heading to Nextion display as serial message ---
    //NEXTION_SERIAL.print("heading=");
    //NEXTION_SERIAL.println(heading);
  }

  // --- SMALL DELAY TO REDUCE CPU USAGE ---
  delay(100);
}
