#pragma once

#include <Arduino.h>
#include <DallasTemperature.h>

namespace tempcontroller {

enum class AddressParseStatus : uint8_t {
    kOk = 0,
    kInvalidLength,
    kInvalidCharacter
};

class AddressCodec {
public:
    static AddressParseStatus Parse(const String& input, DeviceAddress out);
    static bool IsOk(AddressParseStatus status);
    static String Format(const DeviceAddress address);

private:
    static int8_t HexNibble(char c);
};

} // namespace tempcontroller
