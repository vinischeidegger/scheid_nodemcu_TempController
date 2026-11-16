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
      mqttEnabled_(false),
      mqttServer_(""),
      mqttPort_(1883),
      mqttUsername_(""),
      mqttPassword_(""),
      mqttBaseTopic_("tempcontroller"),
      mqttPublishIntervalMs_(2000),
      mqttClient_(mqttNetworkClient_),
      lastReadAt_(0),
      lastMqttPublishAt_(0),
      lastMqttConnectAttemptAt_(0),
      hasTemperatureSnapshot_(false) {
    for (size_t i = 0; i < kMaxSensors; ++i) {
        cachedTemperatureC_[i] = DEVICE_DISCONNECTED_C;
    }
}

void TemperatureControllerApp::Setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- Temperature Controller Boot ---");

    configRegistry_.RegisterParameters();
    configManager_.registerParameter("mqtt_enabled", mqttEnabled_, "MQTT Enabled");
    configManager_.registerParameter("mqtt_server", mqttServer_, "MQTT Server");
    configManager_.registerParameter("mqtt_port", mqttPort_, "MQTT Port");
    configManager_.registerParameter("mqtt_username", mqttUsername_, "MQTT Username");
    configManager_.registerParameter("mqtt_password", mqttPassword_, "MQTT Password");
    configManager_.registerParameter("mqtt_base_topic", mqttBaseTopic_, "MQTT Base Topic");
    configManager_.registerParameter("mqtt_publish_interval_ms", mqttPublishIntervalMs_, "MQTT Publish Interval (ms)");

    configManager_.setMdnsHostname("temp-controller");
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
    mqttClient_.setBufferSize(512);

    Serial.println("Initialization complete. Entering main loop.");
}

void TemperatureControllerApp::Loop() {
    configManager_.handle();
    const unsigned long now = millis();

    MaintainMqttConnection(now);
    if (mqttClient_.connected()) {
        mqttClient_.loop();
        PublishSensorReadingsToMqtt(now);
    }

    const unsigned long effectiveIntervalMs =
        (readIntervalMs_ < kMinReadIntervalMs) ? kMinReadIntervalMs : readIntervalMs_;
    if (now - lastReadAt_ < effectiveIntervalMs) {
        return;
    }

    lastReadAt_ = now;
    temperatureLogger_.ReadAndLog(Serial);
    UpdateTemperatureSnapshotFromLastConversion();
}

bool TemperatureControllerApp::IsWifiReadyForMqtt() const {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    const WiFiMode_t mode = WiFi.getMode();
    return mode != WIFI_OFF && mode != WIFI_AP;
}

void TemperatureControllerApp::MaintainMqttConnection(unsigned long now) {
    if (!mqttEnabled_ || mqttServer_.isEmpty() || !IsWifiReadyForMqtt()) {
        if (mqttClient_.connected()) {
            mqttClient_.disconnect();
        }
        return;
    }

    mqttClient_.setServer(mqttServer_.c_str(), static_cast<uint16_t>(mqttPort_));
    if (mqttClient_.connected()) {
        return;
    }

    if (now - lastMqttConnectAttemptAt_ < 5000) {
        return;
    }

    lastMqttConnectAttemptAt_ = now;
    const String clientId = "temp-controller-" + String(ESP.getChipId(), HEX);
    bool connected = false;

    if (mqttUsername_.isEmpty()) {
        connected = mqttClient_.connect(clientId.c_str());
    } else {
        connected = mqttClient_.connect(clientId.c_str(), mqttUsername_.c_str(), mqttPassword_.c_str());
    }

    if (connected) {
        Serial.println("[MQTT] Connected");
    } else {
        Serial.print("[MQTT] Connect failed rc=");
        Serial.println(mqttClient_.state());
    }
}

String TemperatureControllerApp::BuildSensorTemperatureTopic(size_t slotIndex) const {
    String baseTopic = mqttBaseTopic_;
    if (baseTopic.isEmpty()) {
        baseTopic = "tempcontroller";
    }

    while (baseTopic.endsWith("/")) {
        baseTopic.remove(baseTopic.length() - 1);
    }

    return baseTopic + "/sensor/" + String(slotIndex + 1) + "/temperature";
}

void TemperatureControllerApp::PublishSensorReadingsToMqtt(unsigned long now) {
    const unsigned long publishInterval = (mqttPublishIntervalMs_ < 500) ? 500 : mqttPublishIntervalMs_;
    if (now - lastMqttPublishAt_ < publishInterval) {
        return;
    }

    lastMqttPublishAt_ = now;

    bool hasReadableConfiguredSensor = false;
    for (size_t i = 0; i < kMaxSensors; ++i) {
        if (!sensorSlots_[i].enabled) {
            continue;
        }

        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);
        if (AddressCodec::IsOk(parseStatus)) {
            hasReadableConfiguredSensor = true;
            break;
        }
    }

    if (!hasReadableConfiguredSensor) {
        return;
    }

    sensorDiscovery_.RequestTemperatures();

    for (size_t i = 0; i < kMaxSensors; ++i) {
        if (!sensorSlots_[i].enabled) {
            continue;
        }

        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);
        if (!AddressCodec::IsOk(parseStatus)) {
            continue;
        }

        const float temperatureC = sensorDiscovery_.ReadC(configuredAddress);
        if (temperatureC == DEVICE_DISCONNECTED_C) {
            continue;
        }

        const String topic = BuildSensorTemperatureTopic(i);
        const String payload = String(temperatureC, 2);
        const bool ok = mqttClient_.publish(topic.c_str(), payload.c_str(), true);
        if (!ok) {
            Serial.print("[MQTT] Publish failed topic=");
            Serial.println(topic);
        }
    }
}

void TemperatureControllerApp::UpdateTemperatureSnapshotFromLastConversion() {
    for (size_t i = 0; i < kMaxSensors; ++i) {
        cachedTemperatureC_[i] = DEVICE_DISCONNECTED_C;

        if (!sensorSlots_[i].enabled) {
            continue;
        }

        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);
        if (!AddressCodec::IsOk(parseStatus)) {
            continue;
        }

        cachedTemperatureC_[i] = sensorDiscovery_.ReadC(configuredAddress);
    }

    hasTemperatureSnapshot_ = true;
}


void TemperatureControllerApp::AppendSensorStatusItems(JsonArray items) {
    JsonArray mqttRow = items.add<JsonArray>();
    mqttRow.add("MQTT");
    if (!mqttEnabled_) {
        mqttRow.add("Disabled");
    } else if (mqttServer_.isEmpty()) {
        mqttRow.add("Enabled (server not configured)");
    } else if (!IsWifiReadyForMqtt()) {
        mqttRow.add("Waiting for WiFi STA");
    } else if (mqttClient_.connected()) {
        mqttRow.add("Connected");
    } else {
        mqttRow.add("Disconnected");
    }

    const bool hasDiscoveredSensors = sensorDiscovery_.Count() > 0;

    for (size_t i = 0; i < kMaxSensors; ++i) {
        DeviceAddress configuredAddress;
        const AddressParseStatus parseStatus = AddressCodec::Parse(sensorSlots_[i].address, configuredAddress);

        if (!sensorSlots_[i].enabled && !AddressCodec::IsOk(parseStatus)) {
            continue;
        }

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

        if (!AddressCodec::IsOk(parseStatus)) {
            row.add("Not configured");
            continue;
        }

        if (!hasDiscoveredSensors) {
            row.add("No sensor detected");
            continue;
        }

        if (!hasTemperatureSnapshot_) {
            row.add("Pending first read");
            continue;
        }

        const float temperatureC = cachedTemperatureC_[i];
        if (temperatureC == DEVICE_DISCONNECTED_C) {
            row.add("Disconnected");
            continue;
        }

        row.add(String(temperatureC, 2) + " C");
    }
}

} // namespace tempcontroller
