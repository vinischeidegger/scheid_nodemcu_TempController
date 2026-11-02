#pragma once

#include <Arduino.h>
#include <ConfigManager.h>

#include "SensorDiscovery.h"
#include "TemperatureControllerTypes.h"

namespace tempcontroller {

class SensorAutoMapper {
public:
    SensorAutoMapper(ConfigManager& configManager, SensorSlot* sensorSlots, size_t slotCount,
                     const SensorDiscovery& discovery);

    bool MapEmptySlots(Stream& out);

private:
    ConfigManager& configManager_;
    SensorSlot* sensorSlots_;
    size_t slotCount_;
    const SensorDiscovery& discovery_;
};

} // namespace tempcontroller
