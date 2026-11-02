#include "SensorDiscovery.h"

#include <cstring>

#include "AddressCodec.h"

namespace tempcontroller {

SensorDiscovery::SensorDiscovery(DallasTemperature& sensors) : sensors_(sensors), discoveredCount_(0) {}

void SensorDiscovery::Begin() {
    sensors_.begin();
}

size_t SensorDiscovery::Discover(Stream& out) {
    discoveredCount_ = 0;

    for (size_t busIndex = 0; busIndex < kMaxSensors; ++busIndex) {
        DeviceAddress currentAddress;
        if (!sensors_.getAddress(currentAddress, busIndex)) {
            break;
        }

        memcpy(discoveredAddresses_[discoveredCount_], currentAddress, sizeof(DeviceAddress));
        out.print("Detected sensor #");
        out.print(discoveredCount_ + 1);
        out.print(" @ ");
        out.println(AddressCodec::Format(currentAddress));
        ++discoveredCount_;
    }

    if (discoveredCount_ == 0) {
        out.println("No DS18B20 sensors were detected on the OneWire bus.");
    }

    return discoveredCount_;
}

size_t SensorDiscovery::Count() const {
    return discoveredCount_;
}

bool SensorDiscovery::GetDiscoveredAt(size_t index, DeviceAddress out) const {
    if (index >= discoveredCount_) {
        return false;
    }

    memcpy(out, discoveredAddresses_[index], sizeof(DeviceAddress));
    return true;
}

void SensorDiscovery::RequestTemperatures() {
    sensors_.requestTemperatures();
}

float SensorDiscovery::ReadC(const DeviceAddress address) {
    return sensors_.getTempC(address);
}

} // namespace tempcontroller
