#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using ProtocolValue =
    std::variant<uint8_t, int8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double>;

class AbstractProtocol {
  protected:
    std::vector<uint8_t> construct_packet(const std::vector<uint8_t>& data);

  public:
    std::pair<uint8_t, std::vector<uint8_t>> deconstruct_packet(const std::vector<uint8_t>& packet);

    std::vector<uint8_t> create_rejection_packet();

    virtual std::map<std::string, ProtocolValue> interpret_data(const std::vector<uint8_t>& data) { return {}; };

    virtual ~AbstractProtocol() = 0;
};
