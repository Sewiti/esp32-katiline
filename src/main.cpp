#include <Arduino.h>
#include <DateTime.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WebServer.h>

#include <Config.h>
#include <Logging.h>
#include <TemperatureMonitor.h>

void logTemp();

String info;
Logging *hist;
TemperatureMonitor *monitor;
Ticker *histTicker;
WebServer *server;

void setup()
{
    Serial.begin(115200);
    sleep(1);

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
    info += F("\",\"lastBoot\":\"");
    info += time;
    info += F("\"}");

    // Server
    server = new WebServer(80);
    server->onNotFound([]()
                       { server->send(404, F("text/plain"), F("Not Found")); });
    server->enableCORS();

    // Temperature monitor
    monitor = new TemperatureMonitor(server, TEMP_PIN, TEMP_POLLING, F(TEMP_URI));

    // Temperature history
    hist = new Logging(&SPIFFS, "/test", HIST_FILES, HIST_PER_FILE);
    histTicker = new Ticker(logTemp, HIST_FREQ, 0, MILLIS);
    histTicker->start();

    String path, uri;
    for (int i = 0; i < HIST_FILES; i++)
    {
        uri = String("/hist/" + String(i));
        path = hist->getPath(i);
        server->serveStatic(uri.c_str(), SPIFFS, path.c_str(), "no-cache");
    }

    // Extra endpoints
    server->serveStatic("/", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);
    server->serveStatic("/index.html", SPIFFS, INDEX_FS_PATH, STATIC_CACHE_CONTROL);

    server->on("/info", HTTP_GET, []()
               { server->send(200, F("application/json"), info); });

    server->begin();
}

void loop()
{
    server->handleClient();
    monitor->update();
    histTicker->update();
}

void logTemp()
{
    float tempC = monitor->getTempC();

    if (tempC == DEVICE_DISCONNECTED_C)
        return;

    String line = DateTime.format("%F %H:%M");
    line += ',';
    line += String(tempC, 1);

    hist->log(line.c_str());
}
