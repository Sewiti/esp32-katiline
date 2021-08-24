#include <Arduino.h>
#include <ArduinoJson.h>
#include <DateTime.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Ticker.h>

#include <Config.h>
#include <Logging.h>
#include <TemperatureMonitor.h>

#define CONTENT_HTML "text/html"
#define CONTENT_CSS "text/css"
#define CONTENT_XML "text/xml"
#define CONTENT_JS "application/javascript"
#define CONTENT_JSON "application/json"
#define CONTENT_MANIFEST "application/manifest+json"

#define CONTENT_PNG "image/png"
#define CONTENT_ICO "image/x-icon"
#define CONTENT_SVG "image/svg+xml"

#define NO_CACHE_CONTROL "no-store, max-age=0"
#define STATIC_CACHE_CONTROL "public, max-age=604800" // 7 days

#define SERVE_GZ(path, contentType)                                                    \
    server.on(path, HTTP_GET, [](AsyncWebServerRequest *request) {                     \
        auto response = request->beginResponse(SPIFFS, F(path ".gz"), F(contentType)); \
        response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));              \
        response->addHeader(F("Content-Encoding"), F("gzip"));                         \
        request->send(response);                                                       \
    });

void logTemp();
void checkWiFi();

AsyncWebServer server(80);
TemperatureMonitor monitor(TEMP_PIN, TEMP_POLLING);
Ticker histTicker(logTemp, HIST_FREQ, 0, MILLIS);
Ticker wifiTicker(checkWiFi, WIFI_CHECK_FREQ, 0, MILLIS);

char bootTime[32];
Logging *hist; // Has to be initialized after SPIFFS

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Filesystem
    if (!SPIFFS.begin())
    {
        Serial.println(F("Failed to mount SPIFFS."));
        return;
    }

    // Network
    if (!WiFi.config(
            IPAddress(ADDRESS),
            IPAddress(GATEWAY),
            IPAddress(NETMASK),
            IPAddress(DNS_PRI),
            IPAddress(DNS_ALT)))
        Serial.println(F("Failed to configure Static IP."));

    Serial.print(F("Connecting to "));
    Serial.print(F(SSID));

    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(F("."));
        delay(1000);
    }

    Serial.print(F("\nConnected: "));
    Serial.println(WiFi.localIP());

    // Setup auto WiFi auto reconnect
    wifiTicker.start();

    // Temperature monitoring
    monitor.begin();
    hist = new Logging(&SPIFFS, HIST_PATH, HIST_KEEP_SOFT, HIST_KEEP_HARD);
    histTicker.start();

    // Time
    Serial.print(F("Getting time"));
    DateTime.setTimeZone(TIMEZONE);
    while (!DateTime.begin())
    {
        Serial.print(F("."));
        delay(1000);
    }
    strcpy(bootTime, DateTime.format(DateFormatter::ISO8601).c_str());
    Serial.print(F("\nCurrent time: "));
    Serial.println(bootTime);

    // Server
    DefaultHeaders::Instance().addHeader(F("Access-Control-Allow-Origin"), F("*"));

    // Static files
    // Website
    SERVE_GZ(INDEX_URI, CONTENT_HTML);
    server.rewrite("/", INDEX_URI);

    SERVE_GZ(CHARTJS_URI, CONTENT_JS);
    SERVE_GZ(MAIN_URI, CONTENT_JS);
    SERVE_GZ(STYLES_URI, CONTENT_CSS);

    // Icons
    SERVE_GZ("/android-chrome-192x192.png", CONTENT_PNG);
    SERVE_GZ("/android-chrome-512x512.png", CONTENT_PNG);
    SERVE_GZ("/apple-touch-icon.png", CONTENT_PNG);
    SERVE_GZ("/favicon-16x16.png", CONTENT_PNG);
    SERVE_GZ("/favicon-32x32.png", CONTENT_PNG);
    SERVE_GZ("/mstile-150x150.png", CONTENT_PNG);

    SERVE_GZ("/favicon.ico", CONTENT_ICO);
    SERVE_GZ("/safari-pinned-tab.svg", CONTENT_SVG);

    SERVE_GZ("/browserconfig.xml", CONTENT_XML);
    SERVE_GZ("/site.webmanifest", CONTENT_MANIFEST);

    // API Endpoints
    server.on(INFO_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponseStream(CONTENT_JSON);
                  StaticJsonDocument<192> info;
                  info["commit"] = GIT_COMMIT;
                  info["compileTime"] = COMPILE_TIME;
                  info["lastBoot"] = bootTime;
                  info["systemTime"] = DateTime.format(DateFormatter::ISO8601);
                  serializeJson(info, *response);
                  request->send(response);
              });

    server.on(TEMP_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponseStream(CONTENT_JSON);
                  StaticJsonDocument<20> json;
                  auto temp = monitor.getTempC();
                  if (temp != DEVICE_DISCONNECTED_C)
                      json[TEMP_KEY] = temp;
                  else
                      json[TEMP_KEY] = nullptr;
                  serializeJson(json, *response);
                  request->send(response);
              });

    server.serveStatic(HIST_URI, SPIFFS, HIST_PATH, NO_CACHE_CONTROL);

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, F("text/plain"), F("404 page not found")); });

    server.begin();
}

void loop()
{
    monitor.update();
    histTicker.update();
    wifiTicker.update();
}

void logTemp()
{
    auto temp = monitor.getTempC();
    if (temp == DEVICE_DISCONNECTED_C)
        return;

    String line = DateTime.format("%F %H:%M") + ',' + String(temp, 1);
    hist->log(line.c_str());
}

void checkWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
        WiFi.reconnect();
}
