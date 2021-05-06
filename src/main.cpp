#include <Arduino.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <Secrets.h>
#include <Ticker.h>

#define TEMP_PIN 4
#define READ_TEMP_MILLIS 5000    // 5sec
#define RESTART_MILLIS 259200000 // 3 days

void readTemp();
void allowCors(AsyncWebServerRequest *request);
void setupNetwork();
void setupServer();
void resetServer();

float tempC = DEVICE_DISCONNECTED_C;

OneWire wire(TEMP_PIN);
DallasTemperature sensors(&wire);
DeviceAddress tempAddr;

AsyncWebServer server(80);

Ticker tempTicker(readTemp, READ_TEMP_MILLIS, 0, MILLIS);
Ticker serverTicker(resetServer, RESTART_MILLIS, 0, MILLIS);

void readTemp()
{
    sensors.requestTemperatures();
    const float reading = sensors.getTempC(tempAddr);

    if (reading != DEVICE_DISCONNECTED_C)
        tempC = reading;
}

void allowCors(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader(F("Access-Control-Allow-Origin"), F("*"));
    response->addHeader(F("Access-Control-Allow-Methods"), F("GET"));
    request->send(response);
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
        {
            request->send(200, F("application/json"), F("{\"temp_c\":null}"));
        }
        else
        {
            String Content = F("{\"temp_c\":");
            Content += String(tempC, 1);
            Content += F("}");
            request->send(200, F("application/json"), Content);
        }
    });

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);

        String Content = F("{\"allocated_blocks\":");
        Content += String(info.allocated_blocks);

        Content += F(",\"free_blocks\":");
        Content += String(info.free_blocks);

        Content += F(",\"largest_free_block\":");
        Content += String(info.largest_free_block);

        Content += F(",\"minimum_free_bytes\":");
        Content += String(info.minimum_free_bytes);

        Content += F(",\"total_allocated_bytes\":");
        Content += String(info.total_allocated_bytes);

        Content += F(",\"total_blocks\":");
        Content += String(info.total_blocks);

        Content += F(",\"total_free_bytes\":");
        Content += String(info.total_free_bytes);
        Content += F("}");

        request->send(200, F("application/json"), Content);
    });

    // CORS
    server.on("/temp", HTTP_OPTIONS, allowCors);
    server.on("/heap", HTTP_OPTIONS, allowCors);
}

void resetServer()
{
    server.reset();
    setupServer();
    server.begin();
}

void setup()
{
    Serial.begin(115200);

    // Network setup
    setupNetwork();

    // Sensors setup
    sensors.begin();

    if (!sensors.getAddress(tempAddr, 0))
        Serial.println(F("Unable to find address for Device 0"));

    // Start server
    setupServer();
    server.begin();

    // Start ticker
    tempTicker.start();
    serverTicker.start();
}

void loop()
{
    tempTicker.update();
    serverTicker.update();
}
