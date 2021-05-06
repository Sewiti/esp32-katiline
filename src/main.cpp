#include <Arduino.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <Secrets.h>

#define TEMP_PIN 4

void readTemp();
void setupNetwork();
void setupServer();

float tempC = DEVICE_DISCONNECTED_C;

OneWire wire(TEMP_PIN);
DallasTemperature sensors(&wire);
DeviceAddress tempAddr;

AsyncWebServer server(80);

void readTemp()
{
    sensors.requestTemperatures();
    const float reading = sensors.getTempC(tempAddr);

    if (reading != DEVICE_DISCONNECTED_C)
        tempC = reading;
}

void setupNetwork()
{
    // Addresses
    const IPAddress local_ip(192, 168, 10, 54);
    const IPAddress gateway(192, 168, 10, 1);
    const IPAddress subnet(255, 255, 255, 0);
    const IPAddress dns1(1, 1, 1, 1);
    const IPAddress dns2(1, 0, 0, 1);

    if (!WiFi.config(local_ip, gateway, subnet, dns1, dns2))
        Serial.println(F("Failed to configure Static IP."));

    Serial.print(F("Connecting to "));
    Serial.print(Secrets::wifiSSID);

    WiFi.begin(Secrets::wifiSSID, Secrets::wifiPassphrase);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(F("."));
        delay(1000);
    }

    Serial.print(F("\nConnected: "));
    Serial.println(WiFi.localIP());
}

void setupServer()
{
    // Routes
    server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (tempC == DEVICE_DISCONNECTED_C)
            request->send(200, F("application/json"), F("{\"temp_c\":null}"));

        else
            request->send(200, F("application/json"), "{\"temp_c\":" + String(tempC, 1) + "}");
    });

    // CORS
    auto allowGet = [](AsyncWebServerRequest *request) {
        auto res = request->beginResponse(200);

        res->addHeader(F("Access-Control-Allow-Origin"), F("*"));
        res->addHeader(F("Access-Control-Allow-Methods"), F("GET"));

        request->send(res);
    };

    server.on("/temp", HTTP_OPTIONS, allowGet);
}

void setup()
{
    Serial.begin(115200);

    setupNetwork();

    sensors.begin();
    if (!sensors.getAddress(tempAddr, 0))
        Serial.println(F("Unable to find address for Device 0"));

    setupServer();
    server.begin();
}

void loop()
{
    readTemp();
}
