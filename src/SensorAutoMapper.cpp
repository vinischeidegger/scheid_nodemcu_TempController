#include "SensorAutoMapper.h"

#include "AddressCodec.h"

namespace tempcontroller {

SensorAutoMapper::SensorAutoMapper(ConfigManager& configManager, SensorSlot* sensorSlots, size_t slotCount,
                                   const SensorDiscovery& discovery)
    : configManager_(configManager), sensorSlots_(sensorSlots), slotCount_(slotCount), discovery_(discovery) {}

bool SensorAutoMapper::MapEmptySlots(Stream& out) {
    bool changed = false;

    for (size_t i = 0; i < slotCount_ && i < discovery_.Count(); ++i) {
        if (!sensorSlots_[i].address.isEmpty()) {
            continue;
        }

        DeviceAddress discoveredAddress;
        if (!discovery_.GetDiscoveredAt(i, discoveredAddress)) {
            continue;
        }

        sensorSlots_[i].address = AddressCodec::Format(discoveredAddress);
        changed = true;
        out.print("Auto-mapped slot ");
        out.print(i + 1);
        out.print(" to ");
        out.println(sensorSlots_[i].address);
    }

    if (changed) {
        configManager_.saveConfig();
        out.println("Saved auto-mapped sensor addresses.");
    }

    return changed;
}

} // namespace tempcontroller
