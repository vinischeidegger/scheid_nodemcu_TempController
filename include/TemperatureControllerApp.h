#pragma once

#include <Arduino.h>
#include <ConfigManager.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>

#include "SensorAutoMapper.h"
#include "SensorConfigRegistry.h"
#include "SensorDiscovery.h"
#include "TemperatureControllerTypes.h"
#include "TemperatureLogger.h"

namespace tempcontroller {

class TemperatureControllerApp {
public:
    TemperatureControllerApp();

    void Setup();
    void Loop();

private:
    ConfigManager configManager_;
    SensorSlot sensorSlots_[kMaxSensors];
    unsigned long readIntervalMs_;

    OneWire oneWire_;
    DallasTemperature sensors_;

    SensorConfigRegistry configRegistry_;
    SensorDiscovery sensorDiscovery_;
    SensorAutoMapper autoMapper_;
    TemperatureLogger temperatureLogger_;

    unsigned long lastReadAt_;
};

} // namespace tempcontroller
