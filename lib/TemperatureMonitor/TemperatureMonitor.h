#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include <DateTime.h>
#include <DallasTemperature.h>
#include <Logging.h>
#include <Ticker.h>
#include <WebServer.h>

class TemperatureMonitor
{
protected:
    float tempC;
    Ticker *tempTicker;

    OneWire wire;
    DallasTemperature sensors;
    DeviceAddress tempAddr;

    Logging *logging;
    WebServer *server;

    void readTemp();
    void tempHandler();

public:
    TemperatureMonitor(WebServer *server, const int &pin, const int &tempPollMillis, const Uri &uri = F("/temp"));
    ~TemperatureMonitor();

    float getTempC();
    void update();
};

#endif
