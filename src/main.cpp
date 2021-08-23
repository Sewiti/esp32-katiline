#include <Arduino.h>
#include <ArduinoJson.h>
#include <DateTime.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Ticker.h>

#include <Config.h>
#include <Logging.h>
#include <TemperatureMonitor.h>

#ifndef COMPILE_TIME
#define COMPILE_TIME ""
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT ""
#endif

#define CONTENT_HTML F("text/html")
#define CONTENT_CSS F("text/css")
#define CONTENT_JS F("application/javascript")
#define CONTENT_JSON F("application/json")

#define NO_CACHE_CONTROL "no-store, max-age=0"
#define STATIC_CACHE_CONTROL "public, max-age=604800" // 7 days

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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponse(SPIFFS, F(INDEX_GZ_PATH), CONTENT_HTML);
                  response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));
                  response->addHeader(F("Content-Encoding"), F("gzip"));
                  request->send(response);
              });
    server.rewrite(INDEX_URI, "/");

    server.on(STYLES_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponse(SPIFFS, F(STYLES_GZ_PATH), CONTENT_CSS);
                  response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));
                  response->addHeader(F("Content-Encoding"), F("gzip"));
                  request->send(response);
              });

    server.on(MAIN_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponse(SPIFFS, F(MAIN_GZ_PATH), CONTENT_JS);
                  response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));
                  response->addHeader(F("Content-Encoding"), F("gzip"));
                  request->send(response);
              });

    server.on(CHARTJS_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponse(SPIFFS, F(CHARTJS_GZ_PATH), CONTENT_JS);
                  response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));
                  response->addHeader(F("Content-Encoding"), F("gzip"));
                  request->send(response);
              });

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
