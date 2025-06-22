// ==== Nextion Display Head Unit for IC-905 BLE Server ====
// ---- Receives heading/status notifications from BLE, sends commands from Nextion ----
// ---- EXTREMELY GRANULAR COMMENTARY VERSION ----

// --- Include required libraries for BLE and hardware serial ---
#include <BLEDevice.h>            // BLE core functionality for ESP32
#include <BLEUtils.h>             // BLE utility functions (UUID parsing, etc.)
#include <BLEScan.h>              // BLE scanner to find BLE servers
#include <BLEAdvertisedDevice.h>  // For handling and parsing discovered BLE devices
#include <HardwareSerial.h>       // For secondary hardware serial (Nextion display)

// --- Define TX/RX pins for Serial2 to Nextion display (adjust to your wiring) ---
#define TXD2 17                    // ESP32 GPIO17 to Nextion RX
#define RXD2 16                    // ESP32 GPIO16 to Nextion TX

// --- Initialize Serial2 for Nextion (hardware serial port 2) ---
HardwareSerial nextion(2);         // Use UART2 for Nextion display

// --- BLE UUID Definitions (must match your BLE server) ---
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0" // Main service
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1" // Notifications (server->client)
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2" // Commands (client->server)

// --- BLE client state variables ---
static boolean doConnect = false;                 // Flag: initiate connection
static BLEAdvertisedDevice* myDevice = nullptr;   // Pointer to discovered BLE server
BLEClient* pClient = nullptr;                     // BLE client object pointer
BLERemoteCharacteristic* pCommandCharacteristic = nullptr; // For sending commands to server
BLERemoteCharacteristic* pStatusCharacteristic = nullptr;  // For receiving server notifications
bool connected = false;                           // BLE connection state

// --- Nextion UI component names (adjust if your HMI uses other names) ---
const char* headingTextName = "tHeading"; // Text field for compass heading
const char* statusTextName  = "tStatus";  // Text field for relay/button status

// =============================================================================
// SECTION 1: BLE DEVICE DISCOVERY CALLBACK
// =============================================================================

// --- This class is called when a BLE device is discovered during scan ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Check if this device advertises the service UUID we want
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().toString() == SERVICE_UUID) {
      myDevice = new BLEAdvertisedDevice(advertisedDevice); // Save device pointer
      doConnect = true;                                     // Set flag to connect in loop()
      BLEDevice::getScan()->stop();                         // Stop scan now that device is found
    }
  }
};

// =============================================================================
// SECTION 2: NEXTION HELPER FUNCTIONS
// =============================================================================

// --- Send text value to a named Nextion text component (e.g. tHeading, tStatus) ---
void updateNextionText(const char* component, const String& value) {
  // Format: component.txt="value"
  nextion.print(component);
  nextion.print(".txt=\"");
  nextion.print(value);
  nextion.print("\"");
  // Always end Nextion command with three 0xFF bytes
  nextion.write(0xFF); nextion.write(0xFF); nextion.write(0xFF);
}

// --- Parse heading from BLE status message: looks for "heading: <num>" pattern ---
int parseHeading(const String& msg) {
  int idx = msg.indexOf("heading:");
  if (idx == -1) return -1; // Not found
  int start = idx + 8; // skip "heading:"
  while (start < msg.length() && isspace(msg[start])) start++; // skip whitespace
  int end = start;
  while (end < msg.length() && isDigit(msg[end])) end++;
  String headingStr = msg.substring(start, end);
  return headingStr.toInt();
}

// --- Parse and display status message from BLE server on Nextion fields ---
void handleIncomingStatus(const String& statusMsg) {
  // Extract heading and update heading field
  int heading = parseHeading(statusMsg);
  if (heading != -1) {
    updateNextionText(headingTextName, String(heading)); // Update heading
    Serial.print("[Nextion] Updated heading: "); Serial.println(heading);
  }
  // Extract relay/button status (after first semicolon)
  int semi = statusMsg.indexOf(';');
  String statusOnly = statusMsg;
  if (semi != -1) {
    statusOnly = statusMsg.substring(semi + 1);
    statusOnly.trim();
  }
  updateNextionText(statusTextName, statusOnly); // Update status
  Serial.print("[Nextion] Updated status: "); Serial.println(statusOnly);
}

// =============================================================================
// SECTION 3: BLE CONNECTION AND NOTIFICATION HANDLING
// =============================================================================

// --- Connects to BLE server, sets up command and status characteristics, notifications ---
// Returns true on success, false on error
bool connectToServer() {
  pClient = BLEDevice::createClient();   // Create BLE client instance
  if (!pClient->connect(myDevice)) return false; // Connect to discovered BLE device

  // Get the main service by UUID
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (!pRemoteService) return false;

  // Get command characteristic (for sending commands)
  pCommandCharacteristic = pRemoteService->getCharacteristic(COMMAND_CHAR_UUID);
  if (!pCommandCharacteristic) return false;

  // Get status characteristic (for receiving notifications)
  pStatusCharacteristic = pRemoteService->getCharacteristic(STATUS_CHAR_UUID);
  if (!pStatusCharacteristic) return false;

  // --- Register notification callback using a lambda as required by ESP32 BLE v3.x+ ---
  pStatusCharacteristic->registerForNotify(
    [](BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      // Convert received data to String for processing
      String msg;
      for (size_t i = 0; i < length; i++) msg += (char)pData[i];
      Serial.print("[BLE] Notification: "); Serial.println(msg);
      handleIncomingStatus(msg); // Update Nextion fields
    }
  );

  connected = true; // Mark BLE as connected
  return true;
}

// =============================================================================
// SECTION 4: NEXTION COMMAND MAPPING AND BLE SENDING
// =============================================================================

// --- Map Nextion command strings to BLE command strings (must match server commands) ---
String mapNextionToBLE(const String& cmd) {
  if (cmd == "MF") return "motor right";
  if (cmd == "MR") return "motor left";
  if (cmd == "MS") return "motor stop";
  if (cmd == "CP") return "control power";
  if (cmd == "144ON" || cmd == "144OFF") return "relay 1";
  if (cmd == "440ON" || cmd == "440OFF") return "relay 2";
  if (cmd == "1296ON" || cmd == "1296OFF") return "relay 3";
  if (cmd == "2304ON" || cmd == "2304OFF") return "relay 4";
  if (cmd == "5760ON" || cmd == "5760OFF") return "relay 5";
  // Add more mappings if you add Nextion buttons/fields
  return "";
}

// --- Sends a BLE command string to the server characteristic ---
void sendBLECommand(const String& mappedCmd) {
  if (connected && pCommandCharacteristic) {
    pCommandCharacteristic->writeValue(mappedCmd.c_str(), mappedCmd.length());
    Serial.print("[BLE] Sent command: "); Serial.println(mappedCmd);
  } else {
    Serial.println("[BLE] Not connected! Could not send command.");
  }
}

// =============================================================================
// SECTION 5: ARDUINO SETUP
// =============================================================================

void setup() {
  Serial.begin(115200); // Debug output to USB
  nextion.begin(115200, SERIAL_8N1, RXD2, TXD2); // Nextion serial at 115200 baud

  Serial.println("[Nextion Head Unit] Booting...");

  // Show initial message on Nextion fields
  updateNextionText(headingTextName, "----");
  updateNextionText(statusTextName, "Waiting BLE...");

  // BLE: Initialize and start scanning for servers advertising our service UUID
  BLEDevice::init(""); // No device name needed for client
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Active scan: more data, quicker connect
  pBLEScan->start(30, false);    // Scan for 30 seconds, stop when found
}

// =============================================================================
// SECTION 6: ARDUINO MAIN LOOP
// =============================================================================

void loop() {
  // --- Handle BLE connection if our device was found ---
  if (doConnect && myDevice != nullptr) {
    if (connectToServer()) {
      Serial.println("[BLE] Connected, notifications enabled.");
    } else {
      Serial.println("[BLE] Connection failed, will retry.");
    }
    doConnect = false; // Reset connect flag
  }

  // --- Process input from Nextion display (ASCII text commands) ---
  if (nextion.available()) {
    String data = "";
    delay(30); // Allow buffer to fill for multi-byte commands
    while (nextion.available()) data += (char)nextion.read();
    data.trim(); // Remove whitespace and trailing \r/\n
    if (data.length() > 0) {
      Serial.print("[Nextion] Command from Nextion: "); Serial.println(data);
      String bleCmd = mapNextionToBLE(data);
      if (bleCmd.length() > 0) {
        sendBLECommand(bleCmd);
      } else {
        Serial.println("[Nextion] Unknown Nextion command, ignored.");
      }
    }
  }

  // --- BLE reconnection: if lost connection, restart scan immediately ---
  if (connected && !pClient->isConnected()) {
    connected = false;
    Serial.println("[BLE] Disconnected, restarting scan.");
    BLEDevice::getScan()->start(0); // Start scan again, 0 = continuous
  }

  delay(20); // Small delay to reduce CPU usage
}

// =============================================================================
// SECTION 7: NOTES FOR CUSTOMIZATION AND TROUBLESHOOTING
// =============================================================================
/*
  - Update 'headingTextName' and 'statusTextName' above to match your Nextion project component names.
  - Nextion buttons must send ASCII commands matching the mapNextionToBLE() table.
  - BLE notifications must provide a string in the format:
      "heading: 123; relay 1 on; relay 2 off; relay 3 on; ..."
  - To add new commands/buttons, extend mapNextionToBLE().
  - If your Nextion uses a different baud rate or Serial port, update nextion.begin() and #defines.
  - All BLE notification handling is now done with a lambda function as required by newer ESP32 core.
*/
