#include "../../include/protocol/abstract_protocol.hpp"

// Fix for python implementation
AbstractProtocol::~AbstractProtocol() = default;

std::vector<uint8_t> AbstractProtocol::constructPacket(const std::vector<uint8_t>& data) {
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
    encryptionHandler->encryptData(crcDataBuffer, encryptedBuffer, ivBuffer);

    // Combine Encrypted data and IV
    encryptedBuffer.insert(encryptedBuffer.end(), ivBuffer.begin(), ivBuffer.end());

    // -- Encode in COBS
    std::vector<uint8_t> packet = cobsEncode(encryptedBuffer);

    // Resize to actual encoded size
    return packet;
}

std::pair<bool, std::vector<uint8_t>> AbstractProtocol::deconstructPacket(const std::vector<uint8_t>& packet) {
    std::vector<uint8_t> data;

    // -- Decode COBS
    std::vector<uint8_t> encryptedAndIVMessage = cobsDecode(packet);

    // -- Decrypt
    // Check sizing
    if (encryptedAndIVMessage.size() < (16 + IV_SIZE)) {
        // TODO: Handle error (Packet is too small for one encrypted block and IV)
        return {false, {0}};
    }

    if ((encryptedAndIVMessage.size() - IV_SIZE) % 16 != 0) {
        // TODO: Handle error (Encrypted data size is not a multiple of 16 bytes)
        return {false, {1}};
    }

    const size_t encryptedDataSize = encryptedAndIVMessage.size() - IV_SIZE;

    // Get data
    std::vector<uint8_t> encryptedData(encryptedAndIVMessage.begin(), encryptedAndIVMessage.begin() + encryptedDataSize);
    
    // Get IV
    std::vector<uint8_t> IV;
    IV.reserve(IV_SIZE);

    auto iv_start = encryptedAndIVMessage.begin() + encryptedDataSize;

    IV.insert(IV.end(), iv_start, encryptedAndIVMessage.end());
    
    data.resize(encryptedDataSize);

    encryptionHandler->decryptData(encryptedData, data, IV);

    // -- Check CRC (little endian)
    if (data.size() < CRC_SIZE) {
        // TODO: Handle error (Decrypted data is smaller than CRC size)
        return {false, {2}};
    }

    uint16_t crc = static_cast<uint16_t>(data[1] << 8) | data[0];

    // Remove CRC from data
    std::shift_left(data.begin(), data.end(), CRC_SIZE);
    data.resize(data.size() - CRC_SIZE);

    bool crcCheckResult = checkCRC(crc, data);
    if (!crcCheckResult) {
        // TODO: Handle error
        return {false, {3}};
    }

    return {true, data};
}

std::vector<uint8_t> AbstractProtocol::createRejectionPacket() {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(Protocol::OutgoingHeader::MalformedPacketNotification)
    };

    return constructPacket(data);
}