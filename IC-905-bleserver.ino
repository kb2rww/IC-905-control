// ==== IC-905 BLE Server with QMC5883L Magnetometer True North Heading ====

// --- INCLUDE REQUIRED LIBRARIES ---
// BLE libraries for ESP32 BLE server functionality
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

// QMC5883L Compass library for GY-271 sensor
#include <QMC5883LCompass.h>

// --- BLE SERVICE & CHARACTERISTIC UUIDs ---
// (MUST match the client code for interoperability)
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1"
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2"

// --- BLE Server Pointers ---
BLEServer* pServer = nullptr;             // Pointer to BLE server instance
BLEService* pService = nullptr;           // Pointer to main BLE service
BLECharacteristic* pStatusChar = nullptr; // Pointer to status notify characteristic (server->client)
BLECharacteristic* pCommandChar = nullptr;// Pointer to command write characteristic (client->server)

// --- Connection State Variable ---
bool deviceConnected = false;             // Tracks if BLE client is connected

// --- COMMANDS AND STATE TRACKING ---
// List of supported commands/buttons (must match client exactly)
#define NUM_COMMANDS 8
const char* COMMANDS[NUM_COMMANDS] = {
  "control power",    // Button 1
  "motor left",       // Button 2
  "motor right",      // Button 3
  "relay 1",          // Button 4
  "relay 2",          // Button 5
  "relay 3"           // Button 6
  "relay 4"           // Button 7
  "relay 5"           // Button 8
};
// Array to track ON/OFF state for each command: false = OFF, true = ON
bool commandStates[NUM_COMMANDS] = {false, false, false, false, false, false, false, false};

// --- QMC5883L MAGNETOMETER SENSOR SETUP ---
// Create a compass object for the GY-271 QMC5883L sensor
QMC5883LCompass compass;

// --- YOUR LOCAL MAGNETIC DECLINATION (IN DEGREES) ---
// Find your location's value here: https://www.ngdc.noaa.gov/geomag/calculators/magcalc.shtml
// Example: -7.5 means subtract 7.5° to convert from magnetic north to true north
const float MAGNETIC_DECLINATION = -12.66; // CHANGE THIS FOR YOUR LOCATION home is -12.66

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

    // Special command: PING (reply with PONG)
    if (value == "PING") {
      pStatusChar->setValue("PONG");
      pStatusChar->notify();
      Serial.println("[BLE] Sent PONG response");
      return;
    }

    // Special command: READ_HEADING (client requests compass heading now)
    if (value == "READ_HEADING") {
      String headingMsg = getCompassHeadingString();
      pStatusChar->setValue(headingMsg.c_str());
      pStatusChar->notify();
      Serial.print("[BLE] Sent heading (on demand): ");
      Serial.println(headingMsg);
      return;
    }

    // Find if the command matches one of the known commands/buttons
    int idx = getCommandIndex(value);
    if (idx >= 0) {
      // Toggle the ON/OFF state for the command/button
      commandStates[idx] = !commandStates[idx];
      // Format notification message: e.g., "relay 1 on" or "relay 1 off"
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

// --- GET COMPASS HEADING STRING FUNCTION ---
// Reads the QMC5883L sensor and returns a string with TRUE NORTH heading
String getCompassHeadingString() {
  compass.read(); // Fetch latest sensor values

  // Get raw axis data (for advanced use; not needed here)
  int x = compass.getX();
  int y = compass.getY();
  int z = compass.getZ();

  // Get heading in degrees (magnetic north)
  int magHeading = compass.getAzimuth(); // 0-359 degrees

  // Apply declination correction to get true north heading
  float trueHeading = magHeading + MAGNETIC_DECLINATION;
  // Ensure trueHeading is in 0-359 range
  if (trueHeading < 0) trueHeading += 360;
  if (trueHeading >= 360) trueHeading -= 360;

  // Compose a clear message for the client
  //String msg = "HEADING:mag=" + String(magHeading) + ";true=" + String(trueHeading, 1);
  String msg = "HEADING:" + String(trueHeading, 1); // sends "HEADING:115.5"
  return msg;
}

void setup() {
  // --- SERIAL DEBUGGING SETUP ---
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE Server...");
  Serial.println("[COMPASS] Initializing QMC5883L...");

  // --- INIT QMC5883L COMPASS ---
  compass.init();
  // Optionally, set calibration for your sensor here if needed
  // compass.setCalibration(xOffset, yOffset, scale);

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
  // --- PERIODIC STATUS NOTIFICATION (Shows all states every 3 seconds) ---
  static unsigned long lastNotify = 0;
  if (deviceConnected && millis() - lastNotify > 3000) {
    lastNotify = millis();

    // Build a status string for all commands/buttons: e.g., "relay 1 on; relay 2 off; ..."
    String msg;
    for (int i = 0; i < NUM_COMMANDS; i++) {
      msg += String(COMMANDS[i]) + (commandStates[i] ? " on" : " off");
      if (i != NUM_COMMANDS - 1) msg += "; ";
    }
    // Send the status as a notification to the client
    pStatusChar->setValue(msg.c_str());
    pStatusChar->notify();
    Serial.print("[BLE] Status notified: ");
    Serial.println(msg);
  }

  // --- PERIODIC COMPASS HEADING NOTIFICATION (Every 2 seconds) ---
  static unsigned long lastCompassUpdate = 0;
  if (deviceConnected && millis() - lastCompassUpdate > 2000) {
    lastCompassUpdate = millis();
    String headingMsg = getCompassHeadingString();
    pStatusChar->setValue(headingMsg.c_str());
    pStatusChar->notify();
    Serial.print("[BLE] Sent heading: ");
    Serial.println(headingMsg);
  }

  // --- SMALL DELAY TO REDUCE CPU USAGE ---
  delay(100);
}
