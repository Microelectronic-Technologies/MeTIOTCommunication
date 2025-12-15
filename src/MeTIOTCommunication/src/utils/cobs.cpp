#include "../../include/utils/cobs.hpp"

std::vector<uint8_t> cobsEncode(const std::vector<uint8_t>& data) {
  if (data.empty()) {
    return {1};
  }

  std::vector<uint8_t> encodedBuffer;
  encodedBuffer.reserve(data.size() + (data.size() / 254) + 1);

  encodedBuffer.push_back(0);
  size_t codepIndex = 0;
  uint8_t code = 1;

  for (const uint8_t byte : data) {
    if (byte != 0) {
      encodedBuffer.push_back(byte);
      ++code;
    }

    if (byte == 0 || byte == 0xFF) {
      encodedBuffer[codepIndex] = code;

      code = 1;

      codepIndex = encodedBuffer.size();

      encodedBuffer.push_back(0);
    }
  }

  encodedBuffer[codepIndex] = code;

  encodedBuffer.push_back(0x00); // Delim

  return encodedBuffer;
}

std::vector<uint8_t> cobsDecode(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) {
        return {}; // Empty input always results in empty output
    }

    if (buffer.back() != 0x00 || buffer.size() == 1) {
      // TODO: Handle error (no delim)
      std::cout << "Buffer doesn't end in 0x00" << std::endl;
      return {};
    }
    
    std::vector<uint8_t> decoded_data;
    decoded_data.reserve(buffer.size() - 1);

    size_t buffer_index = 0;
    
    while (buffer_index < buffer.size() - 1) {
      uint8_t code = buffer[buffer_index++];
        
      if (code == 0) {
        // TODO: Handle error (Delim early)
        std::cout << "Found delim early" << std::endl;
        return {};
      }
        
      uint8_t block_size = code - 1;

    if (buffer_index + block_size > buffer.size() - 1) {
      // TODO: Handle error
      std::cout << "Block size would overrun the buff before delim" << std::endl;
      return {};
    }

    std::copy(
      buffer.begin() + buffer_index,
      buffer.begin() + buffer_index + block_size,
      std::back_inserter(decoded_data)
    );

    buffer_index += block_size;
        
    if (code != 0xFF) {
      if (buffer_index < buffer.size() - 1) { 
        decoded_data.push_back(0);
      }
    }
  }

  return decoded_data;
}