#pragma once

#include <Arduino.h>
#include <ConfigManager.h>

#include "TemperatureControllerTypes.h"

namespace tempcontroller {

class SensorConfigRegistry {
public:
    SensorConfigRegistry(ConfigManager& configManager, SensorSlot* sensorSlots, size_t slotCount,
                         unsigned long& readIntervalMs);

    void RegisterParameters();
    void PrintConfiguration(Stream& out) const;

private:
    ConfigManager& configManager_;
    SensorSlot* sensorSlots_;
    size_t slotCount_;
    unsigned long& readIntervalMs_;
};

} // namespace tempcontroller
