#include "TemperatureControllerApp.h"

namespace {

tempcontroller::TemperatureControllerApp app;

} // namespace

void setup() {
    app.Setup();
}

void loop() {
    app.Loop();
}
