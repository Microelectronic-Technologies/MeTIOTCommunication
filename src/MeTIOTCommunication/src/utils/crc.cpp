#include "../../include/utils/crc.hpp"

uint16_t calculateCRC(const std::vector<uint8_t>& data) {
    uint16_t A, crc = 0;

    for (size_t i = 0; i < data.size(); i++) {
        A = (crc >> 8) ^ data[i];
        crc = (A << 2) ^ (A << 1) ^ A ^ (crc << 8);
    }

    return crc;
}

bool checkCRC(const uint16_t crc, const std::vector<uint8_t>& data) {
    uint16_t calculatedCRC = calculateCRC(data);
    return calculatedCRC == crc;
}