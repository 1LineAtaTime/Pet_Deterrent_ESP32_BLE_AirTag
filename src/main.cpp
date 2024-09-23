#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <Arduino.h>
#include <credentials.h>

#define LED 2

int CUTOFF_PLANTS = -35;
int CUTOFF_ROOM = -80;
const int CUTOFF = CUTOFF_PLANTS;

void setup() {
		Serial.begin(9600);
		delay(100);
		pinMode(LED, OUTPUT);
		BLEDevice::init("");
		Serial.println("BLE started");
}

void loop() {
	BLEScan *scan = BLEDevice::getScan();
	scan->setActiveScan(true);
	BLEScanResults results = scan->start(1);

	for (int i = 0; i < results.getCount(); i++) {
		BLEAdvertisedDevice device = results.getDevice(i);
		int rssi = device.getRSSI();

		// Air tags Advertising data every 2000ms. Status byte (0x10) may change, but for now we are only interested on this. See https://adamcatley.com/AirTag.html for reference
		char *ManufacturerData = BLEUtils::buildHexData(nullptr, (uint8_t*)device.getManufacturerData().data(), device.getManufacturerData().length());
		if ((ManufacturerData != nullptr) && (strncmp(ManufacturerData, "4c00121910", 10) == 0) && (strlen(ManufacturerData) == 58)) {
			Serial.println(ManufacturerData);
			Serial.println(rssi);
			if (rssi > CUTOFF) {
				Serial.println("Found Air Tag nearby!");
				digitalWrite(LED, HIGH);
				break;
			} 
			else {
				digitalWrite(LED, LOW);
			}
		} 
		else {
			digitalWrite(LED, LOW);
		}
	}
}