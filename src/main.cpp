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

void logTemp();
void checkWiFi();

AsyncWebServer server(80);
TemperatureMonitor monitor(TEMP_PIN, TEMP_POLLING);
Ticker histTicker(logTemp, HIST_FREQ, 0, MILLIS);
Ticker wifiTicker(checkWiFi, WIFI_CHECK_FREQ, 0, MILLIS);

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
    hist = new Logging(&SPIFFS, HIST_PATH, HIST_FILES, HIST_PER_FILE);
    histTicker.start();

    // Time
    Serial.print(F("Getting time"));
    DateTime.setTimeZone(TIMEZONE);
    while (!DateTime.begin())
    {
        Serial.print(F("."));
        delay(1000);
    }

    String time = DateTime.format(DateFormatter::ISO8601);
    Serial.print(F("\nCurrent time: "));
    Serial.println(time);

    // Basic info
    StaticJsonDocument<120> info;
    info["lastBoot"] = time;
    info["compileTime"] = COMPILE_TIME;
    info["commit"] = GIT_COMMIT;
    char infoSerialized[120];
    serializeJson(info, infoSerialized);

    // Server
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponse(SPIFFS, F(INDEX_GZ_PATH), F("text/html"));
                  response->addHeader(F("Cache-Control"), F(STATIC_CACHE_CONTROL));
                  response->addHeader(F("Content-Encoding"), F("gzip"));
                  request->send(response);
              });
    server.rewrite("/index.html", "/");

    server.on(INFO_URI, HTTP_GET, [infoSerialized](AsyncWebServerRequest *request)
              { request->send(200, F("application/json"), infoSerialized); });

    server.on(TEMP_URI, HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  auto response = request->beginResponseStream(F("application/json"));
                  StaticJsonDocument<20> json;
                  auto temp = monitor.getTempC();
                  if (temp != DEVICE_DISCONNECTED_C)
                      json[TEMP_KEY] = temp;
                  else
                      json[TEMP_KEY] = nullptr;
                  serializeJson(json, *response);
                  request->send(response);
              });

    server.serveStatic(HIST_URI, SPIFFS, HIST_PATH, "no-cache");

    server.onNotFound([](AsyncWebServerRequest *request)
                      {
                          auto file = SPIFFS.open(F(NOTFOUND_GZ_PATH));
                          auto response = request->beginResponse(404, F("text/html"), file.readString());
                          file.close();
                          response->addHeader(F("Content-Encoding"), F("gzip"));
                          request->send(response);
                      });

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
