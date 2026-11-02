#include "AddressCodec.h"

namespace tempcontroller {

AddressParseStatus AddressCodec::Parse(const String& input, DeviceAddress out) {
    uint8_t bytes[8] = {0};
    size_t nibbleCount = 0;

    for (size_t i = 0; i < input.length(); ++i) {
        const char c = input.charAt(i);
        const int8_t nibble = HexNibble(c);

        if (nibble >= 0) {
            if (nibbleCount >= 16) {
                return AddressParseStatus::kInvalidLength;
            }

            const size_t byteIndex = nibbleCount / 2;
            if ((nibbleCount & 1U) == 0) {
                bytes[byteIndex] = static_cast<uint8_t>(nibble << 4);
            } else {
                bytes[byteIndex] = static_cast<uint8_t>(bytes[byteIndex] | static_cast<uint8_t>(nibble));
            }
            ++nibbleCount;
            continue;
        }

        if (c == ':' || c == '-' || c == ' ') {
            continue;
        }

        return AddressParseStatus::kInvalidCharacter;
    }

    if (nibbleCount != 16) {
        return AddressParseStatus::kInvalidLength;
    }

    for (size_t i = 0; i < 8; ++i) {
        out[i] = bytes[i];
    }

    return AddressParseStatus::kOk;
}

bool AddressCodec::IsOk(AddressParseStatus status) {
    return status == AddressParseStatus::kOk;
}

String AddressCodec::Format(const DeviceAddress address) {
    char buffer[17] = {0};
    for (size_t i = 0; i < 8; ++i) {
        snprintf(&buffer[i * 2], 3, "%02X", address[i]);
    }
    return String(buffer);
}

int8_t AddressCodec::HexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<int8_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<int8_t>(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<int8_t>(c - 'a' + 10);
    }

    return -1;
}

} // namespace tempcontroller
