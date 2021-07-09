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
#define WIFI_CHECK_FREQ MINUTE

#define SSID "WIFI_SSID"
#define PASS "WIFI_PASS"

// Temp
#define TEMP_KEY "tempC"
#define TEMP_PIN 4
#define TEMP_POLLING 15 * SECOND

// Temp history
#define HIST_PATH "/hist"
#define HIST_TIME_FORMAT "%F %H:%M"
#define HIST_FREQ 5 * MINUTE
#define HIST_FILES 14
#define HIST_PER_FILE 300
// #define HIST_PER_FILE 24 * HOUR / HIST_FREQ // 288

// Server
#define HIST_URI "/hist"
#define TEMP_URI "/temp"
#define INFO_URI "/info"
#define STATIC_CACHE_CONTROL "max-age=86400" // 1day

// Files
#define INDEX_PATH "/index.min.html"
#define INDEX_GZ_PATH "/index.min.html.gz"

#define NOTFOUND_PATH "/404.min.html"
#define NOTFOUND_GZ_PATH "/404.min.html.gz"

#endif
