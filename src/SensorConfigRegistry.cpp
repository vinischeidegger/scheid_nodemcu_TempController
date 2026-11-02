#include "SensorConfigRegistry.h"

namespace tempcontroller {

SensorConfigRegistry::SensorConfigRegistry(ConfigManager& configManager, SensorSlot* sensorSlots, size_t slotCount,
                                           unsigned long& readIntervalMs)
    : configManager_(configManager), sensorSlots_(sensorSlots), slotCount_(slotCount), readIntervalMs_(readIntervalMs) {}

void SensorConfigRegistry::RegisterParameters() {
    configManager_.registerParameter("temp_read_interval_ms", readIntervalMs_, "Read Interval (ms)");

    for (size_t i = 0; i < slotCount_; ++i) {
        sensorSlots_[i].name = "Sensor " + String(i + 1);
        sensorSlots_[i].address = "";
        sensorSlots_[i].enabled = (i == 0);

        const String slotNumber = String(i + 1);
        configManager_.registerParameter("sensor_" + slotNumber + "_name", sensorSlots_[i].name,
                                         "Sensor " + slotNumber + " Name");
        configManager_.registerParameter("sensor_" + slotNumber + "_address", sensorSlots_[i].address,
                                         "Sensor " + slotNumber + " Address");
        configManager_.registerParameter("sensor_" + slotNumber + "_enabled", sensorSlots_[i].enabled,
                                         "Sensor " + slotNumber + " Enabled");
    }
}

void SensorConfigRegistry::PrintConfiguration(Stream& out) const {
    out.println("Configured sensor slots:");
    for (size_t i = 0; i < slotCount_; ++i) {
        out.print("  Slot ");
        out.print(i + 1);
        out.print(" | name=");
        out.print(sensorSlots_[i].name);
        out.print(" | enabled=");
        out.print(sensorSlots_[i].enabled ? "true" : "false");
        out.print(" | address=");
        out.println(sensorSlots_[i].address.isEmpty() ? "<empty>" : sensorSlots_[i].address);
    }
}

} // namespace tempcontroller
