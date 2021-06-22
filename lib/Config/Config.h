#ifndef CONFIG_H
#define CONFIG_H

#include <DateTimeTZ.h>

// Time
#define TIMEZONE TZ_Europe_Vilnius

// Network
#define ADDRESS 192, 168, 10, 54
#define GATEWAY 192, 168, 10, 1
#define NETMASK 255, 255, 255, 0
#define DNS_PRI 1, 1, 1, 3
#define DNS_ALT 1, 0, 0, 3

#define SSID "WIFI_SSID"
#define PASS "WIFI_PASS"

// Sensors
#define TEMP_PIN 4
#define TEMP_POLLING_MILLIS 15000 // 15sec

// Server
#define TEMP_URI "/temp"
#define STATIC_CACHE_CONTROL "max-age=86400" // 1day

// Files
#define INDEX_FS_PATH "/index.min.html"

#endif
