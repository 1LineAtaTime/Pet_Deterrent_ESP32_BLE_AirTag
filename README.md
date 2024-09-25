Mini ESP32 with BLE scaning, WiFi, Alexa integration and OTA through Sinric Pro.

This program transforms an ESP32 into an AirTag detector (or any other BLE / BlueTooth device) and pushes notifications to a dashboard on Sinric Pro, which manages the OTAs and the Alexa integration. 

RAM:   [==        ]  16.5% (used 53988 bytes from 327680 bytes)

Flash: [========= ]  89.4% (used 1171605 bytes from 1310720 bytes)

Modified latest ArduinoBLE library (@ 1.3.7) with makisin's commit https://github.com/arduino-libraries/ArduinoBLE/pull/53/commits/ae4891dc4bfe19903201e94b93b8b24e288e7fa1 to fix manufacturerData() function. I'm sure it's just on my end, but I couldn't make it work with the default ArduinoBLE library.

Tried using Bluedroid (BLEdevice) library but it was too heavy for my CP2102 CH9102F D1 Mini ESP32.

Also tried using NimBLE library, but it seems like the Arduino library is no longer compatible with some MCU's unless using ESP-IDF
