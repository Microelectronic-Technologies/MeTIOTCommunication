#ifndef COBS_HPP
#define COBS_HPP

#include <vector>
#include <cstdint>

size_t cobsEncode (const void *data, size_t length, uint8_t *buffer);

size_t cobsDecode(const uint8_t *buffer, size_t length, void *data);

#endif