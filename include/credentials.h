
// #define LOCATION_ROOM true // uncomment this line if location is room instead of plants

#define WIFI_SSID           "your WiFi name"
#define WIFI_PASS           "your password"

#define APP_KEY           ""   // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx" (Get it from Portal -> Secrets)
#define APP_SECRET        ""   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx" (Get it from Portal -> Secrets)

#ifndef LOCATION_ROOM
    // Sensor for Plants
    #define CONTACT_ID        ""   // Should look like "5dc1564130xxxxxxxxxxxxxx" (Get it from Portal -> Devices)
#else 
    //  Sensor for Room
    #define CONTACT_ID        ""   // Should look like "5dc1564130xxxxxxxxxxxxxx" (Get it from Portal -> Devices)
#endif
    
