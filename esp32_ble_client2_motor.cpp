#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

#define SERVER_NAME "ESP32-IC905"
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define BTN1_CHAR_UUID "12345678-1234-5678-1234-56789abcdea1"
#define BTN2_CHAR_UUID "12345678-1234-5678-1234-56789abcdea2"
#define BTN3_CHAR_UUID "12345678-1234-5678-1234-56789abcdea3"
#define SLIDER1_CHAR_UUID "12345678-1234-5678-1234-56789abcdea4"

BLEClient*  pClient;
BLERemoteCharacteristic* btn1Char = nullptr;
BLERemoteCharacteristic* btn2Char = nullptr;
BLERemoteCharacteristic* btn3Char = nullptr;
BLERemoteCharacteristic* sliderChar = nullptr;

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  pClient = BLEDevice::createClient();

  BLEScan* pScan = BLEDevice::getScan();
  BLEScanResults foundDevices = pScan->start(5);
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    if (d.getName() == SERVER_NAME) {
      pClient->connect(&d);
      break;
    }
  }

  if (pClient->isConnected()) {
    BLERemoteService* pService = pClient->getService(SERVICE_UUID);
    btn1Char = pService->getCharacteristic(BTN1_CHAR_UUID);
    btn2Char = pService->getCharacteristic(BTN2_CHAR_UUID);
    btn3Char = pService->getCharacteristic(BTN3_CHAR_UUID);
    sliderChar = pService->getCharacteristic(SLIDER1_CHAR_UUID);
    Serial.println("Connected to BLE server!");
  } else {
    Serial.println("Failed to connect to server");
  }
}

void loop() {
  if (btn1Char && btn2Char && btn3Char && sliderChar) {
    btn1Char->writeValue((uint8_t*)"\x01", 1);
    btn2Char->writeValue((uint8_t*)"\x00", 1);
    btn3Char->writeValue((uint8_t*)"\x00", 1);
    sliderChar->writeValue((uint8_t*)"\xC0", 1);
    Serial.println("Motor: Forward, speed 192");
    delay(2000);

    btn1Char->writeValue((uint8_t*)"\x00", 1);
    btn2Char->writeValue((uint8_t*)"\x01", 1);
    btn3Char->writeValue((uint8_t*)"\x00", 1);
    sliderChar->writeValue((uint8_t*)"\x80", 1);
    Serial.println("Motor: Backward, speed 128");
    delay(2000);

    btn1Char->writeValue((uint8_t*)"\x00", 1);
    btn2Char->writeValue((uint8_t*)"\x00", 1);
    btn3Char->writeValue((uint8_t*)"\x01", 1);
    sliderChar->writeValue((uint8_t*)"\x00", 1);
    Serial.println("Motor: Stop");
    delay(2000);
  }
}