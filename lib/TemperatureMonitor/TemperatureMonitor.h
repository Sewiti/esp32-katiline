#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include <DallasTemperature.h>
#include <Ticker.h>

class TemperatureMonitor
{
protected:
    float tempC;
    Ticker *tempTicker;

    OneWire wire;
    DallasTemperature sensors;
    DeviceAddress tempAddr;

    void readTemp();

public:
    TemperatureMonitor(uint8_t pin, uint32_t timer);
    ~TemperatureMonitor();

    void begin();
    void update();

    float getTempC();
};

#endif
