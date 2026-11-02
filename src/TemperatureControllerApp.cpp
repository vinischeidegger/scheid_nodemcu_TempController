#include "TemperatureControllerApp.h"

#include "AddressCodec.h"

namespace tempcontroller {

TemperatureControllerApp::TemperatureControllerApp()
    : readIntervalMs_(kDefaultReadIntervalMs),
      oneWire_(kOneWirePin),
      sensors_(&oneWire_),
      configRegistry_(configManager_, sensorSlots_, kMaxSensors, readIntervalMs_),
      sensorDiscovery_(sensors_),
      autoMapper_(configManager_, sensorSlots_, kMaxSensors, sensorDiscovery_),
      temperatureLogger_(sensorSlots_, kMaxSensors, sensorDiscovery_),
      lastReadAt_(0) {}

void TemperatureControllerApp::Setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- Temperature Controller Boot ---");

    configRegistry_.RegisterParameters();

    String dhcpHostname = WiFi.hostname();
    if (dhcpHostname.isEmpty()) {
        dhcpHostname = "temp-controller";
    }

    configManager_.setMdnsHostname(dhcpHostname);
    configManager_.setPageTitle("Temperature Controller Configuration");
    configManager_.setStatusItemsCallback([this](JsonArray items) {
        AppendSensorStatusItems(items);
    });
    configManager_.begin();

    sensorDiscovery_.Begin();
    const size_t discoveredCount = sensorDiscovery_.Discover(Serial);
    const bool autoMapped = autoMapper_.MapEmptySlots(Serial);
    if (discoveredCount > 0 && !autoMapped) {
        Serial.println("No empty slots available for auto-mapping.");
    }
    configRegistry_.PrintConfiguration(Serial);

    Serial.println("Initialization complete. Entering main loop.");
}

void TemperatureControllerApp::Loop() {
    configManager_.handle();

    const unsigned long effectiveIntervalMs =
        (readIntervalMs_ < kMinReadIntervalMs) ? kMinReadIntervalMs : readIntervalMs_;

    const unsigned long now = millis();
    if (now - lastReadAt_ < effectiveIntervalMs) {
        return;
    }

    lastReadAt_ = now;
    temperatureLogger_.ReadAndLog(Serial);
}

void TemperatureControllerApp::AppendSensorStatusItems(JsonArray items) {
    sensorDiscovery_.RequestTemperatures();

    for (size_t i = 0; i < kMaxSensors; ++i) {
        JsonArray row = items.add<JsonArray>();

        String label = sensorSlots_[i].name;
        if (label.isEmpty()) {
            label = "Sensor " + String(i + 1);
        }
        row.add(label + " Temperature");

        if (!sensorSlots_[i].enabled) {
            row.add("Disabled");
            continue;
        }

        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);
        if (!AddressCodec::IsOk(parseStatus)) {
            row.add("Not configured");
            continue;
        }

        const float temperatureC = sensorDiscovery_.ReadC(configuredAddress);
        if (temperatureC == DEVICE_DISCONNECTED_C) {
            row.add("Disconnected");
            continue;
        }

        row.add(String(temperatureC, 2) + " C");
    }
}

} // namespace tempcontroller
