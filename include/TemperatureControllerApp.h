#pragma once

#include <Arduino.h>
#include <ConfigManager.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <PubSubClient.h>

#include "SensorAutoMapper.h"
#include "SensorConfigRegistry.h"
#include "SensorDiscovery.h"
#include "TemperatureControllerTypes.h"
#include "TemperatureLogger.h"

namespace tempcontroller {

class TemperatureControllerApp {
public:
    TemperatureControllerApp();

    void Setup();
    void Loop();

private:
    ConfigManager configManager_;
    SensorSlot sensorSlots_[kMaxSensors];
    unsigned long readIntervalMs_;

    OneWire oneWire_;
    DallasTemperature sensors_;

    SensorConfigRegistry configRegistry_;
    SensorDiscovery sensorDiscovery_;
    SensorAutoMapper autoMapper_;
    TemperatureLogger temperatureLogger_;

    bool mqttEnabled_;
    String mqttServer_;
    uint32_t mqttPort_;
    String mqttUsername_;
    String mqttPassword_;
    String mqttBaseTopic_;
    unsigned long mqttPublishIntervalMs_;

    WiFiClient mqttNetworkClient_;
    PubSubClient mqttClient_;

    unsigned long lastReadAt_;
    unsigned long lastMqttPublishAt_;
    unsigned long lastMqttConnectAttemptAt_;

    float cachedTemperatureC_[kMaxSensors];
    bool hasTemperatureSnapshot_;

    void AppendSensorStatusItems(JsonArray items);
    bool IsWifiReadyForMqtt() const;
    void MaintainMqttConnection(unsigned long now);
    void PublishSensorReadingsToMqtt(unsigned long now);
    String BuildSensorTemperatureTopic(size_t slotIndex) const;
    void UpdateTemperatureSnapshotFromLastConversion();
};

} // namespace tempcontroller
