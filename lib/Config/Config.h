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
#define DNS_PRI 9, 9, 9, 9
#define DNS_ALT 149, 112, 112, 112
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
#define HIST_KEEP_SOFT 864  // 3 days in 5 mins
#define HIST_KEEP_HARD 1008 // 3.5 days in 5 mins

// Server
#define HIST_URI "/hist"
#define TEMP_URI "/temp"
#define INFO_URI "/info"

#define INDEX_URI "/index.html"
#define STYLES_URI "/styles.css"
#define MAIN_URI "/main.js"
#define CHARTJS_URI "/chart-3.5.1.js"

// Files
#define INDEX_GZ_PATH "/public/index.html.gz"
#define STYLES_GZ_PATH "/public/styles.css.gz"
#define MAIN_GZ_PATH "/public/main.js.gz"
#define CHARTJS_GZ_PATH "/public/chart-3.5.1.js.gz"

#endif
