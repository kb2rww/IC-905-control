#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// === BLE SERVICE AND CHARACTERISTIC UUIDS (Must match client) ===
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0" // Main BLE service
#define STATUS_CHAR_UUID    "12345678-1234-5678-1234-56789abcdef1" // Notification characteristic (server -> client)
#define COMMAND_CHAR_UUID   "12345678-1234-5678-1234-56789abcdef2" // Write characteristic (client -> server)

// Declare pointers for server, service, and characteristics
BLEServer* pServer = nullptr;
BLEService* pService = nullptr;
BLECharacteristic* pStatusChar = nullptr;
BLECharacteristic* pCommandChar = nullptr;

// Variable to track connection state
bool deviceConnected = false;

// ================== Server Callback Class ================== //
// Handles client connect/disconnect events
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected");
    // Restart advertising so another client can connect
    BLEDevice::getAdvertising()->start();
  }
};

// ================== Command Characteristic Callback ================== //
// Handles when the client writes to the command characteristic
class CommandCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue(); // Get data written by client

    // Print received data to serial for debugging
    Serial.print("[BLE] Received command: ");
    for (size_t i = 0; i < value.length(); ++i) {
      Serial.print(value[i]);
    }
    Serial.println();

    // You can parse 'value' and act accordingly here!
    // For example, if you expect ASCII commands:
    if (value == "PING") {
      // Respond to a 'PING' command by notifying status
      pStatusChar->setValue("PONG");      // Set status characteristic value
      pStatusChar->notify();              // Notify client of new status
      Serial.println("[BLE] Sent PONG response");
    }
    // Add more command handling as needed
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("[BLE] Starting BLE Server...");

  // 1. Initialize BLE device and set the advertised name
  BLEDevice::init("IC905_BLE_Server");

  // 2. Create the BLE server object and set callback for connect/disconnect
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create the BLE service using the defined UUID
  pService = pServer->createService(SERVICE_UUID);

  // 4. Create the status characteristic (server -> client, notifications)
  pStatusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY // Only notify property (read-only for client)
  );
  // Add descriptor for notifications (required by most clients)
  pStatusChar->addDescriptor(new BLE2902());
  // Optionally set an initial status value
  pStatusChar->setValue("READY");

  // 5. Create the command characteristic (client -> server, write)
  pCommandChar = pService->createCharacteristic(
    COMMAND_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE // Only write property (write-only for client)
  );
  // Set callback to handle incoming writes
  pCommandChar->setCallbacks(new CommandCharCallbacks());

  // 6. Start the BLE service
  pService->start();

  // 7. Start advertising the service so clients can find it
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // Advertise service UUID for discovery
  pAdvertising->setScanResponse(true);        // Optional (allows scan response packets)
  pAdvertising->setMinPreferred(0x06);        // Improve iOS compatibility
  pAdvertising->setMinPreferred(0x12);        // Improve iOS compatibility
  BLEDevice::startAdvertising();

  Serial.println("[BLE] BLE server is now advertising!");
}

void loop() {
  // Example: Periodically notify client with a status update if connected
  static unsigned long lastNotify = 0;
  if (deviceConnected && millis() - lastNotify > 3000) {
    lastNotify = millis();
    String msg = "STATUS: " + String(millis() / 1000) + "s";
    pStatusChar->setValue(msg.c_str()); // Update status characteristic
    pStatusChar->notify();              // Send notification to client
    Serial.print("[BLE] Status notified: ");
    Serial.println(msg);
  }

  delay(100); // Small delay to avoid excessive CPU usage
}