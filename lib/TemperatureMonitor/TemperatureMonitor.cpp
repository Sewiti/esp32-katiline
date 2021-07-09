#include "TemperatureMonitor.h"

TemperatureMonitor::TemperatureMonitor(uint8_t pin, uint32_t timer)
{
    this->tempC = DEVICE_DISCONNECTED_C;
    this->wire = OneWire(pin);
    this->sensors = DallasTemperature(&this->wire);
    this->tempTicker = new Ticker([this]()
                                  { this->readTemp(); },
                                  timer, 0, MILLIS);
}

TemperatureMonitor::~TemperatureMonitor()
{
    this->tempTicker->stop();
    delete this->tempTicker;
}

void TemperatureMonitor::begin()
{
    this->sensors.begin();
    if (!this->sensors.getAddress(this->tempAddr, 0))
        Serial.println(F("Unable to find address for Device 0"));

    this->readTemp();
    this->tempTicker->start();
}

void TemperatureMonitor::update()
{
    this->tempTicker->update();
}

float TemperatureMonitor::getTempC()
{
    return this->tempC;
}

void TemperatureMonitor::readTemp()
{
    this->sensors.requestTemperatures();
    auto reading = this->sensors.getTempC(this->tempAddr);

    if (reading != DEVICE_DISCONNECTED_C)
        this->tempC = reading;
}
