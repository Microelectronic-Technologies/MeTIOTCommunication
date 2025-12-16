#ifndef FISH_TANK_PROTOCOL_HPP
#define FISH_TANK_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "abstract_protocol.hpp"
#include "../utils/byte_conversion.hpp"

class FishTankProtocol : public AbstractProtocol {
    public:
        std::map<std::string, ProtocolValue> interpretData(const std::vector<uint8_t>& data) override;
        
        ~FishTankProtocol() override {}
};

#endif