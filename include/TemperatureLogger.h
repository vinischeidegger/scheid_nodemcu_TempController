#pragma once

#include <Arduino.h>

#include "SensorDiscovery.h"
#include "TemperatureControllerTypes.h"

namespace tempcontroller {

class TemperatureLogger {
public:
    TemperatureLogger(SensorSlot* sensorSlots, size_t slotCount, SensorDiscovery& discovery);

    void ReadAndLog(Stream& out);

private:
    SensorSlot* sensorSlots_;
    size_t slotCount_;
    SensorDiscovery& discovery_;
};

} // namespace tempcontroller
