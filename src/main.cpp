#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ConfigManager.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace {
constexpr uint8_t kOneWirePin = D2;
constexpr size_t kMaxSensors = 6;
constexpr unsigned long kDefaultReadIntervalMs = 5000;

struct SensorSlot {
    String name;
    String address;
    bool enabled;
};

ConfigManager configManager;
SensorSlot sensorSlots[kMaxSensors];
unsigned long readIntervalMs = kDefaultReadIntervalMs;

OneWire oneWire(kOneWirePin);
DallasTemperature sensors(&oneWire);
DeviceAddress discoveredAddresses[kMaxSensors];
size_t discoveredCount = 0;
unsigned long lastReadAt = 0;

bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

String normalizeAddress(const String& input) {
    String normalized;
    normalized.reserve(16);

    for (size_t i = 0; i < input.length(); ++i) {
        const char c = input.charAt(i);
        if (isHexDigit(c)) {
            normalized += static_cast<char>(toupper(c));
        }
    }

    return normalized;
}

bool parseAddress(const String& input, DeviceAddress out) {
    const String normalized = normalizeAddress(input);
    if (normalized.length() != 16) {
        return false;
    }

    for (size_t i = 0; i < 8; ++i) {
        const String octet = normalized.substring(i * 2, i * 2 + 2);
        out[i] = static_cast<uint8_t>(strtoul(octet.c_str(), nullptr, 16));
    }

    return true;
}

String formatAddress(const DeviceAddress address) {
    char buffer[17] = {0};
    for (size_t i = 0; i < 8; ++i) {
        snprintf(&buffer[i * 2], 3, "%02X", address[i]);
    }
    return String(buffer);
}

void registerConfigParameters() {
    configManager.registerParameter("temp_read_interval_ms", readIntervalMs, "Read Interval (ms)");

    for (size_t i = 0; i < kMaxSensors; ++i) {
        sensorSlots[i].name = "Sensor " + String(i + 1);
        sensorSlots[i].address = "";
        sensorSlots[i].enabled = (i == 0);

        const String slotNumber = String(i + 1);
        configManager.registerParameter("sensor_" + slotNumber + "_name", sensorSlots[i].name,
                                        "Sensor " + slotNumber + " Name");
        configManager.registerParameter("sensor_" + slotNumber + "_address", sensorSlots[i].address,
                                        "Sensor " + slotNumber + " Address");
        configManager.registerParameter("sensor_" + slotNumber + "_enabled", sensorSlots[i].enabled,
                                        "Sensor " + slotNumber + " Enabled");
    }
}

void discoverConnectedSensors() {
    discoveredCount = 0;

    for (size_t busIndex = 0; busIndex < kMaxSensors; ++busIndex) {
        DeviceAddress currentAddress;
        if (!sensors.getAddress(currentAddress, busIndex)) {
            break;
        }

        memcpy(discoveredAddresses[discoveredCount], currentAddress, sizeof(DeviceAddress));
        Serial.print("Detected sensor #");
        Serial.print(discoveredCount + 1);
        Serial.print(" @ ");
        Serial.println(formatAddress(currentAddress));
        ++discoveredCount;
    }

    if (discoveredCount == 0) {
        Serial.println("No DS18B20 sensors were detected on the OneWire bus.");
    }
}

void autoMapEmptyAddresses() {
    bool changed = false;

    for (size_t i = 0; i < kMaxSensors && i < discoveredCount; ++i) {
        if (!sensorSlots[i].address.isEmpty()) {
            continue;
        }

        sensorSlots[i].address = formatAddress(discoveredAddresses[i]);
        changed = true;
        Serial.print("Auto-mapped slot ");
        Serial.print(i + 1);
        Serial.print(" to ");
        Serial.println(sensorSlots[i].address);
    }

    if (changed) {
        configManager.saveConfig();
        Serial.println("Saved auto-mapped sensor addresses.");
    }
}

void printSlotConfiguration() {
    Serial.println("Configured sensor slots:");
    for (size_t i = 0; i < kMaxSensors; ++i) {
        Serial.print("  Slot ");
        Serial.print(i + 1);
        Serial.print(" | name=");
        Serial.print(sensorSlots[i].name);
        Serial.print(" | enabled=");
        Serial.print(sensorSlots[i].enabled ? "true" : "false");
        Serial.print(" | address=");
        Serial.println(sensorSlots[i].address.isEmpty() ? "<empty>" : sensorSlots[i].address);
    }
}

void readAndLogTemperatures() {
    sensors.requestTemperatures();

    for (size_t i = 0; i < kMaxSensors; ++i) {
        if (!sensorSlots[i].enabled) {
            continue;
        }

        DeviceAddress configuredAddress;
        if (!parseAddress(sensorSlots[i].address, configuredAddress)) {
            Serial.print("Slot ");
            Serial.print(i + 1);
            Serial.println(" has an invalid or empty address. Skipping.");
            continue;
        }

        const float temperatureC = sensors.getTempC(configuredAddress);
        Serial.print("[");
        Serial.print(sensorSlots[i].name);
        Serial.print("] ");

        if (temperatureC == DEVICE_DISCONNECTED_C) {
            Serial.print("Address ");
            Serial.print(sensorSlots[i].address);
            Serial.println(" is disconnected or not found.");
            continue;
        }

        Serial.print(temperatureC, 2);
        Serial.print(" C (addr=");
        Serial.print(sensorSlots[i].address);
        Serial.println(")");
    }
}
} // namespace

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- Temperature Controller Boot ---");

    registerConfigParameters();

    String dhcpHostname = WiFi.hostname();
    if (dhcpHostname.isEmpty()) {
        dhcpHostname = "temp-controller";
    }

    configManager.setMdnsHostname(dhcpHostname);
    configManager.setPageTitle("Temperature Controller Configuration");
    configManager.begin();

    sensors.begin();
    discoverConnectedSensors();
    autoMapEmptyAddresses();
    printSlotConfiguration();

    Serial.println("Initialization complete. Entering main loop.");
}

void loop() {
    configManager.handle();

    const unsigned long now = millis();
    if (now - lastReadAt < readIntervalMs) {
        return;
    }

    lastReadAt = now;
    readAndLogTemperatures();
}
