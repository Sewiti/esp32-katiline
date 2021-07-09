#include "TemperatureMonitor.h"

TemperatureMonitor::TemperatureMonitor(WebServer *server, const int &pin, const int &tempPollMillis, const Uri &uri)
{
    this->tempC = DEVICE_DISCONNECTED_C;
    this->wire = OneWire(pin);
    this->sensors = DallasTemperature(&this->wire);

    this->sensors.begin();
    if (!this->sensors.getAddress(this->tempAddr, 0))
        Serial.println(F("Unable to find address for Device 0"));

    this->readTemp();
    this->tempTicker = new Ticker([this]()
                                  { this->readTemp(); },
                                  tempPollMillis, 0, MILLIS);
    this->tempTicker->start();

    this->server = server;
    this->server->on(uri, HTTP_GET, [this]()
                     { this->tempHandler(); });
}

TemperatureMonitor::~TemperatureMonitor()
{
    this->tempTicker->stop();
    delete this->tempTicker;
}

float TemperatureMonitor::getTempC()
{
    return this->tempC;
}

void TemperatureMonitor::tempHandler()
{
    if (this->tempC == DEVICE_DISCONNECTED_C)
        this->server->send(200, F("application/json"), F("{\"temp_c\":null}"));

    else
        this->server->send(200, F("application/json"),
                           "{\"temp_c\":" + String(this->tempC, 1) + F("}"));
}

void TemperatureMonitor::readTemp()
{
    this->sensors.requestTemperatures();
    float reading = this->sensors.getTempC(this->tempAddr);

    if (reading != DEVICE_DISCONNECTED_C)
        this->tempC = reading;
}

void TemperatureMonitor::update()
{
    this->tempTicker->update();
}
