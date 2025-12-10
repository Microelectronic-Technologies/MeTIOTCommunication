#include "../../include/protocol/abstract_protocol.hpp"

void AbstractProtocol::constructPacket(const std::vector<uint8_t>& data, std::vector<uint8_t>& packet) {
    std::vector<uint8_t> crcDataBuffer;
    std::vector<uint8_t> encryptedBuffer;
    std::vector<uint8_t> ivBuffer;
    
    // -- Calc CRC
    uint16_t crc = calculateCRC(data);
    uint8_t highByte = (crc >> 8) & 0xFF;
    uint8_t lowByte = crc & 0xFF;

    crcDataBuffer.emplace_back(highByte);
    crcDataBuffer.emplace_back(lowByte);
    
    // Copy in data
    crcDataBuffer.insert(crcDataBuffer.end(), data.begin(), data.end());

    // -- Encrypt
    encryptionHandler.encryptData(crcDataBuffer, encryptedBuffer, ivBuffer);

    // Combine Encrypted data and IV
    encryptedBuffer.insert(encryptedBuffer.end(), ivBuffer.begin(), ivBuffer.end());

    // -- Encode in COBS
    // Allocate size in packet
    const size_t maxEncodedSize = encryptedBuffer.size() + (encryptedBuffer.size() / 254) + 2;

    packet.clear();
    packet.resize(maxEncodedSize);

    // Encode
    size_t encodedSize = cobsEncode(encryptedBuffer.data(), encryptedBuffer.size(), packet.data());

    // Resize to actual encoded size
    packet.resize(encodedSize);
}

bool AbstractProtocol::deconstructPacket(const std::vector<uint8_t>& packet, std::vector<uint8_t>& data) {
    // Decode COBS
    
    // Decrypt

    // Check CRC
}

void AbstractProtocol::createRejectionPacket(std::vector<uint8_t>& packet) {

}