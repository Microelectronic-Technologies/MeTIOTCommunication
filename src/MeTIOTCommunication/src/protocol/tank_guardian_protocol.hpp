#pragma once

#include "abstract_protocol.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class TankGuardianProtocol : public AbstractProtocol {
  public:
    std::map<std::string, ProtocolValue> interpret_data(const std::vector<uint8_t>& data) override;

    ~TankGuardianProtocol() override = default;
};
