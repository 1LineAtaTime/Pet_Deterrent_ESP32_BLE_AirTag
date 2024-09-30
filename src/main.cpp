#define FIRMWARE_VERSION "1.0.4"

#include <Arduino.h>
#include <credentials.h>

#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProContactsensor.h"
#include "OTA/SemVer.h"
#include "OTA/ESP32OTAHelper.h"
#include "Health/HealthDiagnostics.h"

#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

#define LED 2
#define BUZZER 21
#define BAUD_RATE 115200
#define BT_DISCOVER_TIME	10000

#ifndef LOCATION_ROOM
    // Sensor for Plants
   const int CUTOFF = -42;
   const bool checkPhones = false;
#else 
    //  Sensor for Room
    const int CUTOFF = -65;
    const bool checkPhones = true;
#endif

const int debounceTime = 10000;
bool lastContactState = false;
bool startUp = true;
unsigned long lastChange = 0;
unsigned long tagFoundTime = 0;
unsigned long phoneFoundTime = 0;

HealthDiagnostics healthDiagnostics;
BluetoothSerial SerialBT;

bool handleOTAUpdate(const String &url, int major, int minor, int patch, bool forceUpdate) {
    Version currentVersion = Version(FIRMWARE_VERSION);
    Version newVersion = Version(String(major) + "." + String(minor) + "." + String(patch));
    bool updateAvailable = newVersion > currentVersion;

    Serial.print("URL: ");
    Serial.println(url.c_str());
    Serial.print("Current version: ");
    Serial.println(currentVersion.toString());
    Serial.print("New version: ");
    Serial.println(newVersion.toString());
    if (forceUpdate)
        Serial.println("Enforcing OTA update!");

    // Handle OTA update based on forceUpdate flag and update availability
    if (forceUpdate || updateAvailable)
    {
        if (updateAvailable)
        {
            Serial.println("Update available!");
        }

        String result = startOtaUpdate(url);
        if (!result.isEmpty())
        {
            SinricPro.setResponseMessage(std::move(result));
            return false;
        }
        return true;
    }
    else
    {
        String result = "Current version is up to date.";
        SinricPro.setResponseMessage(std::move(result));
        Serial.println(result);
        return false;
    }
}

void handleContactsensor() {
    if (SinricPro.isConnected() == false) {
        Serial.printf("[Sinric Pro]: Not connected...!\r\n");
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(100);
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        startUp = true; // we reset the startUp bool in case we get disconnected. Sometimes the contact sensor on SinricPro shows up as Closed even though it is not
        return;
    }

    unsigned long actualMillis = millis();
    if (actualMillis - lastChange < debounceTime) return; // debounce contact state transitions (same as debouncing a pushbutton). Changed to 2 seconds

    bool actualContactState = digitalRead(LED);   // read actual state of contactsensor
    
    delay(100);
    
    if ((startUp) or (actualContactState != lastContactState)) {         // if state has changed
        Serial.printf("[Sinric Pro]: Contactsensor is %s now\r\n", actualContactState?"closed":"open");
        lastContactState = actualContactState;              // update last known state
        if (!startUp) {
            lastChange = actualMillis;                      // update debounce time
        }
        startUp = false;                                    // reset it, since we are no longer at startup after this
        SinricProContactsensor &myContact = SinricPro[CONTACT_ID]; // get contact sensor device
        myContact.sendContactEvent(actualContactState);      // send event with actual state
    }
}

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

void btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice) {
    unsigned long actualMillis = millis();

    Serial.printf("Found a device asynchronously: %s\n", pDevice->toString().c_str());
    // need to add some kind of tracker to understand when we find X device, and drop it from the list after Y time

    if  (strcmp(pDevice->toString().c_str(), "phone1") == 0) {
        Serial.println("Found a phone!");
        phoneFoundTime = millis();
    }
}

void startBLuetoothDeviceScan() {
    if (!checkPhones) return;

    delay(1000);

    Serial.print("Starting asynchronous discovery... ");
    if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
        Serial.println("Findings will be reported in \"btAdvertisedDeviceFound\"");
        // do we need this if we want to keep scanning?
        // delay(10000);
        // Serial.print("Stopping discoverAsync... ");
        // SerialBT.discoverAsyncStop();
        // Serial.println("stopped");
    } else {
        Serial.println("Error on discoverAsync f.e. not working after a \"connect\"");
    }
}

void scanBLEDevices() {
    unsigned long actualMillis = millis();

	BLEScan *scan = BLEDevice::getScan();
	scan->setActiveScan(true);
	BLEScanResults results = scan->start(1);

	for (int i = 0; i < results.getCount(); i++) {
		BLEAdvertisedDevice device = results.getDevice(i);
		int rssi = device.getRSSI();

    	char *ManufacturerData = BLEUtils::buildHexData(nullptr, (uint8_t*)device.getManufacturerData().data(), device.getManufacturerData().length());
		if ((ManufacturerData != nullptr) && (strncmp(ManufacturerData, "4c00121910", 10) == 0) && (strlen(ManufacturerData) == 58)) {
			if (rssi >= CUTOFF) {
                if (checkPhones) {
                    if (actualMillis - phoneFoundTime <= 5000) { // phone found with the ArTag within the last 5 seconds...
                        Serial.printf("[BLE]: Found AirTag nearby! RSSI: %d, ManufacturerData: %d\r\n", rssi, ManufacturerData);

                        tagFoundTime = millis();
                        digitalWrite(LED, HIGH);
                        return;
                    }
                }
                else { // we do not need to check other Bluetooth Classic devices for Location Plants.                       
                    Serial.printf("[BLE]: Found AirTag nearby! RSSI: %d, ManufacturerData: %d\r\n", rssi, ManufacturerData);

                    tagFoundTime = millis();
                    digitalWrite(LED, HIGH);
                    return;
                }
            }
            else {
                Serial.printf("[BLE]: Found AirTag! RSSI: %d, ManufacturerData: %d\r\n", rssi, ManufacturerData);
            }
        }
    }
    if (actualMillis - tagFoundTime >= debounceTime) {
        digitalWrite(LED, LOW);
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
    BLEDevice::init("");
    delay(1000);
    SerialBT.begin("ESP32test");  //Bluetooth device name
    
    // Keep scanning for Bluetooth Classic devices
    startBLuetoothDeviceScan();
}

void loop() {
    unsigned long actualMillis = millis();

    // Keep scanning for BLE devices
    scanBLEDevices();
    
    delay(100);

    // Beep only for ~2 seconds
    if (digitalRead(LED) && (actualMillis - tagFoundTime <= 2000)) {
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