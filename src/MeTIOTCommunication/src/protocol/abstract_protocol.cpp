#include "../../include/protocol/abstract_protocol.hpp"

// Fix for python implementation
AbstractProtocol::~AbstractProtocol() = default;

std::vector<uint8_t> AbstractProtocol::constructPacket(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> crcDataBuffer;
    
    // -- Calc CRC (little endian)
    uint16_t crc = calculateCRC(data);
    uint8_t highByte = (crc >> 8) & 0xFF;
    uint8_t lowByte = crc & 0xFF;

    crcDataBuffer.emplace_back(lowByte);
    crcDataBuffer.emplace_back(highByte);
    
    // Copy in data
    crcDataBuffer.insert(crcDataBuffer.end(), data.begin(), data.end());

    // -- Encode in COBS
    std::vector<uint8_t> packet = cobsEncode(crcDataBuffer);

    // Resize to actual encoded size
    return packet;
}

std::pair<uint8_t, std::vector<uint8_t>> AbstractProtocol::deconstructPacket(const std::vector<uint8_t>& packet) {
    // -- Decode COBS
    std::vector<uint8_t> headerAndData = cobsDecode(packet);

    // -- Check CRC (little endian)
    if (headerAndData.size() < CRC_SIZE) {
        // TODO: Handle error (Decrypted data is smaller than CRC size)
        return {0, {1}};
    }

    uint16_t crc = static_cast<uint16_t>(headerAndData[1] << 8) | headerAndData[0];

    // Remove CRC from data
    std::shift_left(headerAndData.begin(), headerAndData.end(), CRC_SIZE);
    headerAndData.resize(headerAndData.size() - CRC_SIZE);

    bool crcCheckResult = checkCRC(crc, headerAndData);
    if (!crcCheckResult) {
        // TODO: Handle error
        return {0, {2}};
    }

    // Seperate header and data

    uint8_t header = headerAndData[0];
    std::vector<uint8_t> data(headerAndData.begin() + 1, headerAndData.end());

    return {header, data};
}

std::vector<uint8_t> AbstractProtocol::createRejectionPacket() {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(Protocol::OutgoingHeader::MalformedPacketNotification)
    };

    return constructPacket(data);
}