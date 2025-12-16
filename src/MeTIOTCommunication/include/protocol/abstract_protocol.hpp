#ifndef ABSTRACT_PROTOCOL_HPP
#define ABSTRACT_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <utility>
#include <iostream>
#include <memory>
#include <string>

#include "protocol_constants.hpp"
#include "../utils/crc.hpp"
#include "../utils/cobs.hpp"
#include "../security/aes_encryption.hpp"

class AbstractProtocol {
    private:
        std::unique_ptr<EncryptionHandler> encryptionHandler = nullptr;
        uint64_t deviceID;
        uint32_t sessionSalt;

    protected:
        std::vector<uint8_t> constructPacket(const std::vector<uint8_t>& data);

    public:
        std::pair<bool, std::vector<uint8_t>> deconstructPacket(const std::vector<uint8_t>& packet);

        std::vector<uint8_t> createRejectionPacket();

        void createEncryptionHandler(std::string passPhrase);

        AbstractProtocol(uint64_t deviceID, uint32_t sessionSalt) : deviceID(deviceID), sessionSalt(sessionSalt) {}
        virtual ~AbstractProtocol() = 0;
};

#endif