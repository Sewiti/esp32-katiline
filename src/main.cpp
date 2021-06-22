#include <Arduino.h>
#include <DateTime.h>
#include <SPIFFS.h>
#include <WebServer.h>

#include <Config.h>
#include <TemperatureMonitor.h>

String info;
WebServer *server;
TemperatureMonitor *monitor;

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
    DateTime.setTimeZone(TIMEZONE);
    while (!DateTime.begin())
    {
        Serial.print(F("."));
        delay(1000);
    }

    String time = DateTime.format("%T %b %e %Y");
    Serial.print(F("\nCurrent time: "));
    Serial.println(time);

    // Basic info
    info = F("{\"compiled\":\"");
    info += __TIME__;
    info += F(" ");
    info += __DATE__;
    info += F("\",\"last_boot\":\"");
    info += time;
    info += F("\"}");

    // Server
    server = new WebServer(80);
    monitor = new TemperatureMonitor(server, TEMP_PIN, TEMP_POLLING_MILLIS, F(TEMP_URI));

    server->serveStatic("/", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);
    server->serveStatic("/index.html", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);

    server->on("/info", HTTP_GET, []()
               { server->send(200, F("application/json"), info); });

    server->onNotFound([]()
                       { server->send(404, F("text/plain"), F("Not Found")); });
    server->enableCORS();
    server->begin();
}

void loop()
{
    server->handleClient();
    monitor->update();
}
