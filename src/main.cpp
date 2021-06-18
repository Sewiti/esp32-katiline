#include <Arduino.h>
#include <DateTime.h>
#include <SPIFFS.h>
#include <WebServer.h>

#include <Config.h>
#include <TemperatureMonitor.h>

WebServer server(80);
TemperatureMonitor monitor(&server, TEMP_PIN, TEMP_POLLING_MILLIS, F(TEMP_URI));

void setup()
{
    Serial.begin(115200);

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

    // Time
    Serial.print(F("Getting time"));
    while (!DateTime.begin())
    {
        Serial.print(F("."));
        delay(1000);
    }

    Serial.print(F("\nCurrent time: "));
    Serial.println(DateTime.format(DateFormatter::ISO8601));

    // Server
    server.serveStatic("/", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);
    server.serveStatic("/index.html", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);

    server.onNotFound([]()
                      { server.send(404, F("text/plain"), F("Not Found")); });
    server.enableCORS();
    server.begin();
}

void loop()
{
    server.handleClient();
    monitor.update();
}
