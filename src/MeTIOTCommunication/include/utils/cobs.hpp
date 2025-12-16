#ifndef COBS_HPP
#define COBS_HPP

#include <vector>
#include <cstdint>
#include <iostream>

/* @brief    Encodes the data using COBS
 *
 * @param    data      Data that is being encoded
 * @param    length    Size of the data being encoded
 * @param    buffer    Buffer to store the encoded data (ensure adequate buffer size)
 * 
 * @retval   `size_t` - The real output size of the encoding
 * 
 * @note     This is a c-style function. Its implementation requires the use of raw pointers.
 * @note     This cobs encoding function does NOT add the deliminator.
 */
std::vector<uint8_t> cobs_encode(const std::vector<uint8_t>& data);

/* @brief    Decodes the data using COBS
 *
 * @param    buffer     The encoded data buffer
 * @param    length     Size of the encoded COBS buffer
 * @param    data       Data output after decoding
 * 
 * @retval   `size_t` - The real output size of the encoding
 * 
 * @note     This is a c-style function. Its implementation requires the use of raw pointers.
 */
std::vector<uint8_t> cobs_decode(const std::vector<uint8_t>& buffer);

#endif