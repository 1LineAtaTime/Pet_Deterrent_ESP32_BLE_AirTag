#define FIRMWARE_VERSION "1.0.4"

#include <Arduino.h>
#include <credentials.h>

// Modified latest ArduinoBLE library (@ 1.3.7) with makisin's commit
#include "ArduinoBLE.h"

#include "OTA/SemVer.h"
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProContactsensor.h"
#include "OTA/ESP32OTAHelper.h"
#include "Health/HealthDiagnostics.h"

#define LED 2
#define BUZZER 21
#define BAUD_RATE 115200

#ifndef LOCATION_ROOM
    // Sensor for Plants
   const int CUTOFF = -39;
#else 
    //  Sensor for Room
    const int CUTOFF = -65;
#endif
    
const int debounceTime = 10000;
bool lastContactState = false;
unsigned long lastChange = 0;
unsigned long tagFound = 0;

HealthDiagnostics healthDiagnostics;

bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  Version currentVersion  = Version(FIRMWARE_VERSION);
  Version newVersion      = Version(String(major) + "." + String(minor) + "." + String(patch));
  bool updateAvailable    = newVersion > currentVersion;

  Serial.print("URL: ");
  Serial.println(url.c_str());
  Serial.print("Current version: ");
  Serial.println(currentVersion.toString());
  Serial.print("New version: ");
  Serial.println(newVersion.toString());
  if (forceUpdate) Serial.println("Enforcing OTA update!");

  // Handle OTA update based on forceUpdate flag and update availability
  if (forceUpdate || updateAvailable) {
    if (updateAvailable) {
      Serial.println("Update available!");
    }

    String result = startOtaUpdate(url);
    if (!result.isEmpty()) {
      SinricPro.setResponseMessage(std::move(result));
      return false;
    } 
    return true;
  } else {
    String result = "Current version is up to date.";
    SinricPro.setResponseMessage(std::move(result));
    Serial.println(result);
    return false;
  }
}

void handleContactsensor() {
    if (SinricPro.isConnected() == false) {
        Serial.printf("[Sinric Pro]: Not connected...!\r\n");
        return; 
    }

    unsigned long actualMillis = millis();
    if (actualMillis - lastChange < debounceTime) return;          // debounce contact state transitions (same as debouncing a pushbutton). Changed to 2 seconds

    bool actualContactState = digitalRead(LED);   // read actual state of contactsensor
    
    delay(100);
    
    if (actualContactState != lastContactState) {         // if state has changed
        Serial.printf("[Sinric Pro]: Contactsensor is %s now\r\n", actualContactState?"closed":"open");
        lastContactState = actualContactState;              // update last known state
        lastChange = actualMillis;                          // update debounce time
        SinricProContactsensor &myContact = SinricPro[CONTACT_ID]; // get contact sensor device
        myContact.sendContactEvent(actualContactState);      // send event with actual state
    }
}

// setup function for WiFi connection
void setupWiFi() {
    unsigned long actualMillis = millis();

    Serial.printf("[WiFi]: Connecting");
    WiFi.setAutoReconnect(true);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        Serial.printf("...");
        delay(250);
        if (millis() - actualMillis > 20000)
            return;
    }
    IPAddress localIP = WiFi.localIP();
    Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

// setup function for SinricPro
void setupSinricPro() {
    // add device to SinricPro
    SinricProContactsensor& myContact = SinricPro[CONTACT_ID];

    // setup SinricPro
    SinricPro.onConnected([](){ Serial.printf("[Sinric Pro]: Connected\r\n"); });
    SinricPro.onDisconnected([](){ Serial.printf("[Sinric Pro]: Disconnected\r\n"); });
    SinricPro.onOTAUpdate(handleOTAUpdate);
    SinricPro.onReportHealth([&](String &healthReport) {
        return healthDiagnostics.reportHealth(healthReport);
    });  
    SinricPro.begin(APP_KEY, APP_SECRET);
    
    Serial.printf("[Sinric Pro]: Connecting\r\n");
}

void scanBLEDevices() {
    unsigned long actualMillis = millis();

    // Start scanning for BLE devices
    int scanDuration = 500;  // 500ms seconds scanning period
    
    BLEDevice foundDevice;
    if (BLE.scan(scanDuration)) {
        // Loop through the discovered devices
        while (foundDevice = BLE.available()) {
            if ((foundDevice.manufacturerData().startsWith("4c00121910")) && (foundDevice.manufacturerData().length() == 58)){
                if (foundDevice.rssi() >= CUTOFF) {
                    Serial.printf("[BLE]: Found AirTag nearby! RSSI: %d, ManufacturerData: ", foundDevice.rssi());
                    Serial.print(foundDevice.manufacturerData()); Serial.print("\r\n");

                    tagFound = millis();
                    digitalWrite(LED, HIGH);
                    return;
                }
                else {
                    Serial.printf("[BLE]: Found AirTag! RSSI: %d, ManufacturerData: ", foundDevice.rssi());
                    Serial.print(foundDevice.manufacturerData()); Serial.print("\r\n");
                }
            }
        }
        if (actualMillis - tagFound >= debounceTime) {
            digitalWrite(LED, LOW);
        }
    } else {
        Serial.println("[BLE]: No BLE devices found\r\n");
        if (actualMillis - tagFound >= debounceTime) {
            digitalWrite(LED, LOW);
        }
    }
}

void setup() {
    Serial.begin(BAUD_RATE);
    delay(100);
    Serial.printf("\r\n[ESP32]: Started with Firmware Version: %s\r\n", FIRMWARE_VERSION);
    pinMode(LED, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    
    setupWiFi();

    setupSinricPro();

    delay(2000);
    if (!BLE.begin()){
        Serial.println("[BLE]: Couldn't start\r\n");
    }
    else {
        Serial.println("[BLE]: Started\r\n");
    }
}

void loop() {
    unsigned long actualMillis = millis();

    // Keep scanning for BLE devices
    scanBLEDevices();
    
    delay(100);

    // Beep only for ~2 seconds
    if (digitalRead(LED) && (actualMillis - tagFound <= 2000)) {
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi]: Dropped connection\r\n");
        setupWiFi();
    }

    handleContactsensor();
    SinricPro.handle();
}