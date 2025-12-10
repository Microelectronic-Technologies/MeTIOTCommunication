#ifndef CRC_HPP
#define CRC_HPP

#include <vector>
#include <cstdint>

uint16_t calculateCRC(const std::vector<uint8_t>& data);

bool checkCRC(const uint16_t crc, const std::vector<uint8_t>& data);

#endif