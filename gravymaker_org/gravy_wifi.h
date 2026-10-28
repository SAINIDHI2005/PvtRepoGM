#pragma once

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

void moveSteps(int motorIndex, bool directionCW, int sectorId);
extern int interDelay;
extern Adafruit_PWMServoDriver stepPwm;

//spice check
void spiceCheck();


// Hotspot credentials
static const char* GM_WIFI_SSID = "Galaxy F15 5G";
static const char* GM_WIFI_PASS = "abcd1234";

static constexpr uint16_t GM_WIFI_PORT = 8080;

static WiFiServer gmServer(GM_WIFI_PORT);
static WiFiClient gmClient;
static String gmRxBuffer;

// Defined in your existing GravyMaker sketch.
void feeder(int sel, int cnt);

// Wi-Fi dispensing path: feed only, no chopping or grinding.
inline void gmFeedVegetableOnly(int sel, int servings) {
    Serial.println(sel == 0
        ? "Feeding Onion only..."
        : "Feeding Tomato only...");

    stepPwm.setPWMFreq(1500);

    for (int i = 0; i < servings; i++) {
        Serial.printf("Serving %d of %d\n", i + 1, servings);

        moveSteps(sel, false, 1);
        delay(interDelay);
        moveSteps(sel, true, 1);
    }

    stepPwm.setPWMFreq(50);
    Serial.println("Feeding movement complete.");
}

inline void gmSendStatus(const char* status) {
    if (gmClient && gmClient.connected()) {
        gmClient.println(status);
    }

    Serial.print("[WiFi status] ");
    Serial.println(status);
}

inline void gmWiFiBegin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(GM_WIFI_SSID, GM_WIFI_PASS);

    Serial.print("Connecting to hotspot");

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 20000) {
        delay(300);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("GravyMaker IP: ");
        Serial.println(WiFi.localIP());

        gmServer.begin();
        gmServer.setNoDelay(true);
        Serial.println("GravyMaker TCP server ready on port 8080");
    } else {
        Serial.println("GravyMaker WiFi connection failed");
    }
}

inline void gmExecuteDispense(String request) {
    request.trim();

    int onionPos = request.indexOf("ONION=");
    int tomatoPos = request.indexOf("TOMATO=");
    int servingsPos = request.indexOf("SERVINGS=");

    if (!request.startsWith("DISPENSE|") ||
        onionPos < 0 || tomatoPos < 0 || servingsPos < 0) {
        gmSendStatus("ERROR|BAD_REQUEST");
        return;
    }

    bool onion = request.substring(onionPos + 6).startsWith("1");
    bool tomato = request.substring(tomatoPos + 7).startsWith("1");
    int servings = request.substring(servingsPos + 9).toInt();

    if ((!onion && !tomato) || (servings != 1 && servings != 2)) {
        gmSendStatus("ERROR|INVALID_SELECTION");
        return;
    }

    Serial.println("=== DISPENSE REQUEST ACCEPTED ===");
    Serial.printf("Onion: %d, Tomato: %d, Servings: %d\n",
                  onion, tomato, servings);

    // Onion sequence must finish before tomato begins.
    if (onion) {
        gmSendStatus("ONION_START");
        gmFeedVegetableOnly(0, servings);
        gmSendStatus("ONION_DONE");
    }

    if (tomato) {
        gmSendStatus("TOMATO_START");
        gmFeedVegetableOnly(1, servings);
        gmSendStatus("TOMATO_DONE");
    }

    gmSendStatus("DISPENSE_DONE");
    Serial.println("=== DISPENSE REQUEST COMPLETED ===");
}

inline void gmWiFiLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!gmClient || !gmClient.connected()) {
        WiFiClient incoming = gmServer.available();

        if (incoming) {
            gmClient = incoming;
            gmClient.setNoDelay(true);
            gmRxBuffer = "";
            Serial.println("CrowPanel connected");
        }
    }

    if (!gmClient || !gmClient.connected()) {
        return;
    }

    while (gmClient.available()) {
        char c = (char)gmClient.read();

        if (c == '\n') {
            gmRxBuffer.trim();

            if (gmRxBuffer.length()) {
                Serial.print("CrowPanel request: ");
                Serial.println(gmRxBuffer);
                if (gmRxBuffer == "SPICECHECK") {
                    spiceCheck();
                }
                else if (gmRxBuffer.startsWith("DISPENSE|")) {
                    gmExecuteDispense(gmRxBuffer);
                }
                else {
                    gmSendStatus("ERROR|UNKNOWN_COMMAND");
                }
            }

            gmRxBuffer = "";
        } else if (c != '\r') {
            if (gmRxBuffer.length() < 180) {
                gmRxBuffer += c;
            } else {
                gmRxBuffer = "";
                gmSendStatus("ERROR|REQUEST_TOO_LONG");
            }
        }
    }
}