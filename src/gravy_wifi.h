#pragma once

#include <WiFi.h>

static const char* CROW_WIFI_SSID = "Galaxy F15 5G";
static const char* CROW_WIFI_PASS = "abcd1234";

// Update this with the IP printed by the main GravyMaker ESP32.
static IPAddress MAIN_ESP32_IP(10, 85, 205, 116);
static constexpr uint16_t GRAVY_PORT = 8080;

static WiFiClient gravyClient;
static String gravyRxBuffer;
static bool gravyRequestActive = false;



inline bool beginSpiceCheckRequest() {
    if (gravyRequestActive) {
        Serial.println("A request is already active.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(CROW_WIFI_SSID, CROW_WIFI_PASS);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED &&
               millis() - start < 15000) {
            delay(10);
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("CrowPanel WiFi connection failed.");
        return false;
    }

    gravyClient.stop();

    if (!gravyClient.connect(MAIN_ESP32_IP, GRAVY_PORT)) {
        Serial.println("Could not connect to GravyMaker.");
        return false;
    }

    gravyClient.setNoDelay(true);
    gravyRxBuffer = "";
    gravyClient.println("SPICECHECK");
    gravyRequestActive = true;

    Serial.println("Sent SPICECHECK request.");
    return true;
}


inline bool beginDispenseRequest(
    bool onionSelected,
    bool tomatoSelected,
    int servings
) {
    if (gravyRequestActive) {
        Serial.println("A dispense request is already active.");
        return false;
    }

    if ((!onionSelected && !tomatoSelected) ||
        (servings != 1 && servings != 2)) {
        Serial.println("Invalid ingredient or serving selection.");
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(CROW_WIFI_SSID, CROW_WIFI_PASS);

        Serial.println("Connecting CrowPanel to hotspot...");
        unsigned long start = millis();

        while (WiFi.status() != WL_CONNECTED &&
               millis() - start < 15000) {
            delay(10);
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("CrowPanel WiFi connection failed.");
        return false;
    }

    gravyClient.stop();

    Serial.print("Connecting to GravyMaker at ");
    Serial.println(MAIN_ESP32_IP);

    if (!gravyClient.connect(MAIN_ESP32_IP, GRAVY_PORT)) {
        Serial.println("Could not connect to GravyMaker.");
        return false;
    }

    gravyClient.setNoDelay(true);
    gravyRxBuffer = "";

    String command = "DISPENSE|ONION=";
    command += onionSelected ? "1" : "0";
    command += "|TOMATO=";
    command += tomatoSelected ? "1" : "0";
    command += "|SERVINGS=";
    command += String(servings);

    Serial.print("Sending: ");
    Serial.println(command);

    gravyClient.println(command);
    gravyRequestActive = true;
    return true;
}

// Returns true when one complete status line has been received.
inline bool pollDispenseStatus(String &status) {
    while (gravyClient.available()) {
        char c = (char)gravyClient.read();

        if (c == '\n') {
            status = gravyRxBuffer;
            status.trim();
            gravyRxBuffer = "";

            Serial.print("GravyMaker status: ");
            Serial.println(status);

            if (status == "DISPENSE_DONE" ||
              status == "SPICECHECK_DONE" ||
              status.startsWith("ERROR|")) {
                gravyRequestActive = false;
                gravyClient.stop();
            }

            return true;
        }

        if (c != '\r' && gravyRxBuffer.length() < 100) {
            gravyRxBuffer += c;
        } else if (gravyRxBuffer.length() >= 100) {
            gravyRxBuffer = "";
        }
    }

    // Treat a dropped connection as a failed operation.
    if (gravyRequestActive && !gravyClient.connected() &&
        !gravyClient.available()) {
        status = "ERROR|CONNECTION_LOST";
        gravyRequestActive = false;
        gravyClient.stop();
        return true;
    }

    return false;
}

inline bool dispenseRequestActive() {
    return gravyRequestActive;
}