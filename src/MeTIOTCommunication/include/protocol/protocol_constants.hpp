#ifndef PROTOCOL_CONSTANTS_HPP
#define PROTOCOL_CONSTANTS_HPP

#include <cstdint>

namespace Protocol {
 
    enum class OutgoingHeader : uint8_t {
        MalformedPacketNotification = 0xFF,
        DeviceIdentificationRequest = 0xFE,
    };

    enum class IncomingHeader : uint8_t {
        MalformedPacketNotification = 0xFF,
        DeviceIdentifier            = 0xFE,
        Data                        = 0xFD,
    };

    enum class DeviceIdentifier : uint8_t {
        FishTank                    = 0xFF,
    };

}

#endif