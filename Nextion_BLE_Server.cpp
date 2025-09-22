#include <Arduino.h>
#include <Preferences.h>
#include "EasyNextionLibrary.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define TXD2 17
#define RXD2 16
#define NEXTION_BAUD 115200

#define NUM_PAGE1_BTNS 3
#define NUM_PAGE2_BTNS 10

EasyNex myNex(Serial2);
bool page1Btns[NUM_PAGE1_BTNS] = {false, false, false};
int slider1Value = 0;
bool page2Btns[NUM_PAGE2_BTNS] = {false};
Preferences prefs;

#define SERVICE_UUID           "12345678-1234-5678-1234-56789abcdef0"
#define STATUS_CHAR_UUID       "12345678-1234-5678-1234-56789abcdef1"
#define BTN1_CHAR_UUID         "12345678-1234-5678-1234-56789abcdea1"
#define BTN2_CHAR_UUID         "12345678-1234-5678-1234-56789abcdea2"
#define BTN3_CHAR_UUID         "12345678-1234-5678-1234-56789abcdea3"
#define SLIDER1_CHAR_UUID      "12345678-1234-5678-1234-56789abcdea4"

BLEServer* pServer = nullptr;
BLECharacteristic* statusChar = nullptr;
BLECharacteristic* page1BtnChars[NUM_PAGE1_BTNS];
BLECharacteristic* slider1Char = nullptr;
BLECharacteristic* page2BtnChars[NUM_PAGE2_BTNS];

bool deviceConnected = false;

void saveStates();
void loadStates();
void syncPage1();
void syncPage2();
void logStates();
void notifyStatus();

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    notifyStatus();
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
  }
};

class Btn1CharCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    uint8_t val = *(pChar->getData());
    page1Btns[0] = val;
    saveStates();
    syncPage1();
    logStates();
    notifyStatus();
  }
};
class Btn2CharCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    uint8_t val = *(pChar->getData());
    page1Btns[1] = val;
    saveStates();
    syncPage1();
    logStates();
    notifyStatus();
  }
};
class Btn3CharCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    uint8_t val = *(pChar->getData());
    page1Btns[2] = val;
    saveStates();
    syncPage1();
    logStates();
    notifyStatus();
  }
};
class Slider1CharCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    uint8_t val = *(pChar->getData());
    slider1Value = val;
    saveStates();
    myNex.writeStr("t0.txt", String(slider1Value));
    syncPage1();
    logStates();
    notifyStatus();
  }
};
class Btn2nCharCallback : public BLECharacteristicCallbacks {
public:
  Btn2nCharCallback(int idx_) : idx(idx_) {}
  void onWrite(BLECharacteristic* pChar) override {
    uint8_t val = *(pChar->getData());
    page2Btns[idx] = val;
    saveStates();
    syncPage2();
    logStates();
    notifyStatus();
  }
private:
  int idx;
};

void syncPage1() {
  for (int i = 0; i < NUM_PAGE1_BTNS; i++) {
    String btnName = "btn" + String(i + 1);
    myNex.writeNum(btnName + ".val", page1Btns[i] ? 1 : 0);
  }
  myNex.writeNum("slider1.val", slider1Value);
  myNex.writeStr("t0.txt", String(slider1Value));
}
void syncPage2() {
  for (int i = 0; i < NUM_PAGE2_BTNS; i++) {
    String btnName = "btn2_" + String(i + 1);
    myNex.writeNum(btnName + ".val", page2Btns[i] ? 1 : 0);
  }
}
void saveStates() {
  prefs.begin("ui", false);
  for (int i = 0; i < NUM_PAGE1_BTNS; i++)
    prefs.putBool(("p1b" + String(i)).c_str(), page1Btns[i]);
  prefs.putInt("slider1", slider1Value);
  for (int i = 0; i < NUM_PAGE2_BTNS; i++)
    prefs.putBool(("p2b" + String(i)).c_str(), page2Btns[i]);
  prefs.end();
}
void loadStates() {
  prefs.begin("ui", true);
  for (int i = 0; i < NUM_PAGE1_BTNS; i++)
    page1Btns[i] = prefs.getBool(("p1b" + String(i)).c_str(), false);
  slider1Value = prefs.getInt("slider1", 0);
  for (int i = 0; i < NUM_PAGE2_BTNS; i++)
    page2Btns[i] = prefs.getBool(("p2b" + String(i)).c_str(), false);
  prefs.end();
  logStates();
}
void logStates() {
  Serial.print("Page1 Btns: ");
  for (int i = 0; i < NUM_PAGE1_BTNS; i++)
    Serial.printf("[%d]=%d ", i, page1Btns[i]);
  Serial.printf("| slider1=%d", slider1Value);
  Serial.print(" | Page2 Btns: ");
  for (int i = 0; i < NUM_PAGE2_BTNS; i++)
    Serial.printf("[%d]=%d ", i, page2Btns[i]);
  Serial.println();
}
void notifyStatus() {
  if (!deviceConnected || !statusChar) return;
  String msg = "{";
  msg += "\"btn1\":" + String(page1Btns[0]) + ",";
  msg += "\"btn2\":" + String(page1Btns[1]) + ",";
  msg += "\"btn3\":" + String(page1Btns[2]) + ",";
  msg += "\"slider1\":" + String(slider1Value) + ",";
  msg += "\"page2\":[";
  for (int i = 0; i < NUM_PAGE2_BTNS; i++) {
    msg += String(page2Btns[i]);
    if (i < NUM_PAGE2_BTNS - 1) msg += ",";
  }
  msg += "]}";
  statusChar->setValue(msg.c_str());
  statusChar->notify();
}

void btn1Handler() {
  page1Btns[0] = !page1Btns[0];
  saveStates(); syncPage1(); logStates(); notifyStatus();
  if (page1BtnChars[0]) page1BtnChars[0]->setValue((uint8_t*)&page1Btns[0], 1);
}
void btn2Handler() {
  page1Btns[1] = !page1Btns[1];
  saveStates(); syncPage1(); logStates(); notifyStatus();
  if (page1BtnChars[1]) page1BtnChars[1]->setValue((uint8_t*)&page1Btns[1], 1);
}
void btn3Handler() {
  page1Btns[2] = !page1Btns[2];
  saveStates(); syncPage1(); logStates(); notifyStatus();
  if (page1BtnChars[2]) page1BtnChars[2]->setValue((uint8_t*)&page1Btns[2], 1);
}
void slider1Handler() {
  slider1Value = myNex.readNumber("slider1.val");
  saveStates(); myNex.writeStr("t0.txt", String(slider1Value));
  logStates(); notifyStatus();
  if (slider1Char) slider1Char->setValue((uint8_t*)&slider1Value, 1);
}
void btn2BtnHandler() {
  String comp = myNex.currentComponent;
  int idx = comp.substring(5).toInt() - 1;
  if (idx >= 0 && idx < NUM_PAGE2_BTNS) {
    page2Btns[idx] = !page2Btns[idx];
    saveStates(); syncPage2(); logStates(); notifyStatus();
    if (page2BtnChars[idx]) page2BtnChars[idx]->setValue((uint8_t*)&page2Btns[idx], 1);
  }
}
void page1Handler() { syncPage1(); }
void page2Handler() { syncPage2(); }

void setup() {
  Serial.begin(115200);
  Serial2.begin(NEXTION_BAUD, SERIAL_8N1, RXD2, TXD2);
  myNex.begin(NEXTION_BAUD);

  loadStates();

  BLEDevice::init("ESP32-IC905");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  statusChar = pService->createCharacteristic(
    STATUS_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  statusChar->addDescriptor(new BLE2902());

  page1BtnChars[0] = pService->createCharacteristic(BTN1_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
  page1BtnChars[1] = pService->createCharacteristic(BTN2_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
  page1BtnChars[2] = pService->createCharacteristic(BTN3_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
  slider1Char      = pService->createCharacteristic(SLIDER1_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);

  page1BtnChars[0]->setCallbacks(new Btn1CharCallback());
  page1BtnChars[1]->setCallbacks(new Btn2CharCallback());
  page1BtnChars[2]->setCallbacks(new Btn3CharCallback());
  slider1Char->setCallbacks(new Slider1CharCallback());

  for (int i = 0; i < NUM_PAGE2_BTNS; i++) {
    char uuid[40];
    snprintf(uuid, sizeof(uuid), "12345678-1234-5678-1234-56789abcde%02d", i+1);
    page2BtnChars[i] = pService->createCharacteristic(uuid, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
    page2BtnChars[i]->setCallbacks(new Btn2nCharCallback(i));
  }

  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("BLE Server is running...");
}

void loop() {
  myNex.NextionListen();
  delay(10);
}

EASYNEXTION_EVENT(btn1, btn1Handler);
EASYNEXTION_EVENT(btn2, btn2Handler);
EASYNEXTION_EVENT(btn3, btn3Handler);
EASYNEXTION_EVENT(slider1, slider1Handler);
EASYNEXTION_EVENT(btn2_1, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_2, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_3, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_4, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_5, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_6, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_7, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_8, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_9, btn2BtnHandler);
EASYNEXTION_EVENT(btn2_10, btn2BtnHandler);
EASYNEXTION_EVENT(page1, page1Handler);
EASYNEXTION_EVENT(page2, page2Handler);
