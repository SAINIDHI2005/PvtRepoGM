#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>

namespace gravy_net {
static const char *BLE_DEVICE_NAME = "GravyMakerController";
static BLEUUID SERVICE_UUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID COMMAND_UUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static BLEClient *client = nullptr;
static BLERemoteCharacteristic *commandCharacteristic = nullptr;

class ServerScanCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
    BLEAdvertisedDevice *found = nullptr;
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        if (advertisedDevice.haveName() && advertisedDevice.getName() == BLE_DEVICE_NAME) {
            found = new BLEAdvertisedDevice(advertisedDevice);
            BLEDevice::getScan()->stop();
        }
    }
};

inline bool connectController() {
    if (client && client->isConnected() && commandCharacteristic) return true;
    commandCharacteristic = nullptr;
    BLEDevice::init("CrowPanelGravyUI");
    BLEScan *scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(80);
    ServerScanCallbacks callbacks;
    scan->setAdvertisedDeviceCallbacks(&callbacks, false);
    Serial.println("BLE: scanning for GravyMakerController...");
    scan->start(5, false);
    if (!callbacks.found) {
        Serial.println("BLE: controller not found");
        scan->clearResults();
        return false;
    }
    if (!client) client = BLEDevice::createClient();
    bool connected = client->connect(callbacks.found);
    delete callbacks.found;
    scan->clearResults();
    if (!connected) {
        Serial.println("BLE: connection failed");
        return false;
    }
    BLERemoteService *service = client->getService(SERVICE_UUID);
    if (!service) {
        Serial.println("BLE: expected service not found");
        client->disconnect();
        return false;
    }
    commandCharacteristic = service->getCharacteristic(COMMAND_UUID);
    if (!commandCharacteristic || !commandCharacteristic->canWrite()) {
        Serial.println("BLE: writable command characteristic not found");
        client->disconnect();
        commandCharacteristic = nullptr;
        return false;
    }
    Serial.println("BLE: connected to GravyMakerController");
    return true;
}

inline void begin() {
    // BLE connection is established on first send, avoiding a long scan during boot.
    Serial.println("BLE sender ready; controller connects when Submit is pressed");
}

inline bool sendCommand(const char *command, char *reply, size_t replySize) {
    if (reply && replySize) reply[0] = '\0';
    if (!command || !*command) {
        if (reply && replySize) strlcpy(reply, "ERR|Empty command", replySize);
        return false;
    }
    if (!connectController()) {
        if (reply && replySize) strlcpy(reply, "ERR|BLE controller not found", replySize);
        return false;
    }
    // writeValue() in the ESP32 BLE library returns void; a successful return
    // means the local BLE stack accepted the write (not that motors finished).
    if (commandCharacteristic->canWriteNoResponse())
        commandCharacteristic->writeValue((uint8_t *)command, strlen(command), false);
    else
        commandCharacteristic->writeValue((uint8_t *)command, strlen(command), true);
    Serial.printf("BLE sent: %s\n", command);
    if (reply && replySize) strlcpy(reply, "Sent over BLE", replySize);
    return true;
}
} // namespace gravy_net
