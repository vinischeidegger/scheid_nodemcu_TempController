#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>

#include "TemperatureControllerTypes.h"

namespace tempcontroller {

class SensorDiscovery {
public:
    explicit SensorDiscovery(DallasTemperature& sensors);

    void Begin();
    size_t Discover(Stream& out);

    size_t Count() const;
    bool GetDiscoveredAt(size_t index, DeviceAddress out) const;

    void RequestTemperatures();
    float ReadC(const DeviceAddress address);

private:
    DallasTemperature& sensors_;
    DeviceAddress discoveredAddresses_[kMaxSensors];
    size_t discoveredCount_;
};

} // namespace tempcontroller
