#ifndef BYTE_CONVERSION_HPP
#define BYTE_CONVERSION_HPP

#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

int8_t bytesToInt8(const std::vector<uint8_t>& data, int currentPlace);

uint8_t bytesToUint8(const std::vector<uint8_t>& data, int currentPlace);

int16_t bytesToInt16(const std::vector<uint8_t>& data, int currentPlace);

uint16_t bytesToUint16(const std::vector<uint8_t>& data, int currentPlace);

int32_t bytesToInt32(const std::vector<uint8_t>& data, int currentPlace);

uint32_t bytesToUint32(const std::vector<uint8_t>& data, int currentPlace);

int64_t bytesToInt64(const std::vector<uint8_t>& data, int currentPlace);

uint64_t bytesToUint64(const std::vector<uint8_t>& data, int currentPlace);

float bytesToFloat(const std::vector<uint8_t>& data, int currentPlace);

double bytesToDouble(const std::vector<uint8_t>& data, int currentPlace);

#endif