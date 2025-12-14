#include "../../include/protocol/device_agnostic_protocol.hpp"

std::vector<uint8_t> DeviceAgnosticProtocol::create_device_id_request() {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(Protocol::OutgoingHeader::DeviceIdentificationRequest)
    };

    return constructPacket(data);
}