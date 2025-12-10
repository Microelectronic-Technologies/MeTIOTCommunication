#ifndef FISH_TANK_PROTOCOL_HPP
#define FISH_TANK_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "abstract_protocol.hpp"

class FishTankProtocol : public AbstractProtocol {
    public:
        FishTankProtocol(std::vector<uint8_t>& key) : AbstractProtocol(key) {}
        ~FishTankProtocol() override {}
};

#endif