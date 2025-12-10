#include "../../include/protocol/abstract_protocol.hpp"

void AbstractProtocol::constructPacket(const std::vector<uint8_t>& data, std::vector<uint8_t>& packet) {
    std::vector<uint8_t> crcDataBuffer;
    std::vector<uint8_t> encryptedBuffer;
    std::vector<uint8_t> ivBuffer;
    
    // -- Calc CRC (little endian)
    uint16_t crc = calculateCRC(data);
    uint8_t highByte = (crc >> 8) & 0xFF;
    uint8_t lowByte = crc & 0xFF;

    crcDataBuffer.emplace_back(lowByte);
    crcDataBuffer.emplace_back(highByte);
    
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
    // -- Decode COBS
    std::vector<uint8_t> encryptedAndIVMessage(packet.size());
    
    size_t decodedSize = cobsDecode(packet.data(), packet.size(), encryptedAndIVMessage.data());

    encryptedAndIVMessage.resize(decodedSize);

    // -- Decrypt
    // Check sizing
    if ((encryptedAndIVMessage.size() - IV_SIZE) % 16 != 0) {
        // TODO: Handle error
        return false;
    }
    const size_t blockCount = (encryptedAndIVMessage.size() - IV_SIZE) / 16;

    std::vector<uint8_t> encryptedData(encryptedAndIVMessage.begin(), encryptedAndIVMessage.begin() + (blockCount * 16));
    std::vector<uint8_t> IV(encryptedAndIVMessage.begin() + (blockCount * 16), encryptedAndIVMessage.end());

    data.resize(blockCount * 16);

    bool err = encryptionHandler.decryptData(encryptedData, data, IV);
    if (!err) {
        // TODO: Handle error
        return false;
    }

    // -- Check CRC (little endian)
    uint16_t crc = static_cast<uint16_t>(data[1] << 8) | data[0];

    // Remove CRC from data
    std::shift_left(data.begin(), data.end(), CRC_SIZE);
    data.resize(data.size() - CRC_SIZE);

    bool crcCheckResult = checkCRC(crc, data);
    if (!crcCheckResult) {
        // TODO: Handle error
        return false;
    }

    return true;
}

void AbstractProtocol::createRejectionPacket(std::vector<uint8_t>& packet) {

}