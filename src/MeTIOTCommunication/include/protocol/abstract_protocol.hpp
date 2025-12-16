#ifndef ABSTRACT_PROTOCOL_HPP
#define ABSTRACT_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <variant>

#include "protocol_constants.hpp"
#include "../utils/crc.hpp"
#include "../utils/cobs.hpp"

using ProtocolValue = std::variant<
    uint8_t,
    int8_t,
    int16_t,
    uint16_t,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    float,
    double
>;

class AbstractProtocol {
    private:

    protected:
        std::vector<uint8_t> constructPacket(const std::vector<uint8_t>& data);

    public:
        std::pair<uint8_t, std::vector<uint8_t>> deconstructPacket(const std::vector<uint8_t>& packet);

        std::vector<uint8_t> createRejectionPacket();

        virtual std::map<std::string, ProtocolValue> interpretData(const std::vector<uint8_t>& data) { return {}; };

        virtual ~AbstractProtocol() = 0;
};

#endif