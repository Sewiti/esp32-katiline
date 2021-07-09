#ifndef CONFIG_H
#define CONFIG_H

#include <DateTimeTZ.h>

// Millisecond helpers
#define SECOND 1000
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

// Timezone
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
#define TEMP_POLLING 15 * SECOND

// Temp history
#define HIST_FREQ 5 * MINUTE
#define HIST_FILES 14
#define HIST_PER_FILE 300
// #define HIST_PER_FILE 24 * HOUR / HIST_FREQ // 288

// Server
#define TEMP_URI "/temp"
#define STATIC_CACHE_CONTROL "max-age=86400" // 1day

// Files
#define INDEX_FS_PATH "/index.min.html"

#endif
