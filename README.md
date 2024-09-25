Mini ESP32 with BLE scanning, WiFi, Alexa integration and OTA through Sinric Pro.

This program transforms an ESP32 into an AirTag detector (or any other BLE / Bluetooth device) and pushes notifications to a dashboard on Sinric Pro, which manages the OTAs and the Alexa integration. 

There are a myriad of use cases for this device. I personally created it to know when my cats decide to start eating my plants or go into a room they are not supposed to be in. Both of my cats have an AirTag on their collar, in case one day they escape, so I can get an idea of what neighborhood they could be in. I then use that same air tags to detect if they are nearby. I have a spare iPhone I used to set them up on the Find My network, but then shut it off. The anti-stalker feature of the AirTag makes it advertise its presence every 2 seconds when they've been out of range of the parent phone for a while. Both the Address() and ManufacturerData() of the AirTag changes periodically, but we can extract the first few characters of the ManufacturerData() to identify it. More info on the Advertising Data can be found on https://adamcatley.com/AirTag.html

Other cases could be setting a trap door that only gets activated by a specific type of BLE or Bluetooth device (e.g. backyard dog trap door), etc.


https://portal.sinric.pro/dashboard can be used to manage the devices (see status, history, OTA, etc). I used this tutorial (https://help.sinric.pro/pages/tutorials/contact-sensors/contact) to set up my ESP32 to act as a contact sensor, then added a routine on Alexa so it sends me a notification to my phone and it announces it to the Echo Show when the contact sensor closes (i.e. AirTag is within range)


Notes:

RAM:   [==        ]  16.5% (used 53988 bytes from 327680 bytes)

Flash: [========= ]  89.4% (used 1171605 bytes from 1310720 bytes)

Modified latest ArduinoBLE library (@ 1.3.7) with makisin's commit https://github.com/arduino-libraries/ArduinoBLE/pull/53/commits/ae4891dc4bfe19903201e94b93b8b24e288e7fa1 to fix manufacturerData() function. I'm sure it's just on my end, but I couldn't make it work with the default ArduinoBLE library.

Tried using Bluedroid (BLEdevice) library but it was too heavy for my CP2102 CH9102F D1 Mini ESP32.

Also tried using NimBLE library, but it seems like the Arduino library is no longer compatible with some MCU's unless using ESP-IDF
