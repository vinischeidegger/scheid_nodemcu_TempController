#include "TemperatureLogger.h"

#include "AddressCodec.h"

namespace tempcontroller {

TemperatureLogger::TemperatureLogger(SensorSlot* sensorSlots, size_t slotCount, SensorDiscovery& discovery)
    : sensorSlots_(sensorSlots), slotCount_(slotCount), discovery_(discovery) {}

void TemperatureLogger::ReadAndLog(Stream& out) {
    discovery_.RequestTemperatures();

    for (size_t i = 0; i < slotCount_; ++i) {
        if (!sensorSlots_[i].enabled) {
            continue;
        }

        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);
        if (!AddressCodec::IsOk(parseStatus)) {
            out.print("Slot ");
            out.print(i + 1);
            if (parseStatus == AddressParseStatus::kInvalidCharacter) {
                out.println(" has an address with invalid characters. Skipping.");
            } else {
                out.println(" has an invalid or empty address length. Skipping.");
            }
            continue;
        }

        const float temperatureC = discovery_.ReadC(configuredAddress);
        out.print("[");
        out.print(sensorSlots_[i].name);
        out.print("] ");

        if (temperatureC == DEVICE_DISCONNECTED_C) {
            out.print("Address ");
            out.print(sensorSlots_[i].address);
            out.println(" is disconnected or not found.");
            continue;
        }

        out.print(temperatureC, 2);
        out.print(" C (addr=");
        out.print(sensorSlots_[i].address);
        out.println(")");
    }
}

} // namespace tempcontroller
