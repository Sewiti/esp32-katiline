#include <Arduino.h>
#include <DallasTemperature.h>
#include <Secrets.h>
#include <SPIFFS.h>
#include <Ticker.h>
#include <WebServer.h>

// #define DEBUG
#define PROD

#define TEMP_PIN 4
#define TEMP_POLLING_MS 15000 // 15sec

#define INDEX_HTML_PATH "/index.min.html"
#define CACHE_CONTROL "max-age=1800"

void setupNetwork();
void readTemp();
void handleTemp();
void handleNotFound();

float tempC = DEVICE_DISCONNECTED_C;

OneWire wire(TEMP_PIN);
DallasTemperature sensors(&wire);
DeviceAddress tempAddr;

WebServer server(80);
Ticker tempTicker(readTemp, TEMP_POLLING_MS, 0, MILLIS);

void setupNetwork()
{
    // Addresses
#ifdef PROD
    IPAddress local_ip(192, 168, 10, 54);
#else
    IPAddress local_ip(192, 168, 10, 55);
#endif

    IPAddress gateway(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns1(1, 1, 1, 1);
    IPAddress dns2(1, 0, 0, 1);

    if (!WiFi.config(local_ip, gateway, subnet, dns1, dns2))
        Serial.println(F("Failed to configure Static IP."));

    Serial.print(F("Connecting to "));
    Serial.print(F(Secrets::wifiSSID));

    WiFi.begin(Secrets::wifiSSID, Secrets::wifiPassphrase);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(F("."));
        delay(1000);
    }

    Serial.print(F("\nConnected: "));
    Serial.println(WiFi.localIP());
}

void readTemp()
{
#ifdef DEBUG
    Serial.println(F("readTemp()"));
#endif

    sensors.requestTemperatures();
    float reading = sensors.getTempC(tempAddr);

    if (reading != DEVICE_DISCONNECTED_C)
    {
#ifdef DEBUG
        Serial.print(F("temp_c: "));
        Serial.println(reading);
#endif

        tempC = reading;
    }
}

void handleTemp()
{
#ifdef DEBUG
    Serial.println(F("handleTemp()"));
#endif

    if (tempC == DEVICE_DISCONNECTED_C)
        server.send(200, F("application/json"), F("{\"temp_c\":null}"));

    else
        server.send(200, F("application/json"), "{\"temp_c\":" + String(tempC, 1) + "}");
}

void handleNotFound()
{
#ifdef DEBUG
    Serial.println(F("handleNotFound()"));
#endif

    server.send(404, F("text/plain"), F("Not Found"));
}

void setup()
{
    Serial.begin(115200);

    if (SPIFFS.begin())
    {
        server.serveStatic("/", SPIFFS, INDEX_HTML_PATH, CACHE_CONTROL);
        server.serveStatic("/index.html", SPIFFS, INDEX_HTML_PATH, CACHE_CONTROL);
    }
    else
        Serial.println(F("Failed to mount SPIFFS."));

    setupNetwork();

    sensors.begin();
    if (!sensors.getAddress(tempAddr, 0))
        Serial.println(F("Unable to find address for Device 0"));

    readTemp();
    tempTicker.start();

    server.on("/temp", HTTP_GET, handleTemp);
    server.onNotFound(handleNotFound);
    server.enableCORS();
    server.begin();

#ifdef DEBUG
    Serial.println(F("setup() exit"));
#endif
}

void loop()
{
    server.handleClient();
    tempTicker.update();
}
