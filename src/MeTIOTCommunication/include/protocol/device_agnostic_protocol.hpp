/*

What is the purpose of a "Device Agnostic Protocol"?
- During device initialization before we know the device type but do
- know our shared ECDH key we must still use function from within the
- AbstractProtocol class to construct our messages in the correct
- format.
- This class allows you to create a temporary class for before a type
- is confirmed.

*/

#ifndef DEVICE_AGNOSTIC_PROTOCOL_HPP
#define DEVICE_AGNOSTIC_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "abstract_protocol.hpp"

class DeviceAgnosticProtocol : public AbstractProtocol {
    public:
        DeviceAgnosticProtocol(std::vector<uint8_t>& key) : AbstractProtocol(key) {}
        ~DeviceAgnosticProtocol() override {}

        std::vector<uint8_t> create_device_id_request();
};

#endif