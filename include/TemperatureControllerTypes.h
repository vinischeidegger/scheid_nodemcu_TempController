#pragma once

#include <Arduino.h>

namespace tempcontroller {

constexpr uint8_t kOneWirePin = D2;
constexpr size_t kMaxSensors = 6;
constexpr unsigned long kDefaultReadIntervalMs = 5000;
constexpr unsigned long kMinReadIntervalMs = 250;

struct SensorSlot {
    String name;
    String address;
    bool enabled;
};

} // namespace tempcontroller
