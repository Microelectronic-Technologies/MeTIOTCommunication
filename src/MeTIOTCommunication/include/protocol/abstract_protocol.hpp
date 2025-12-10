#ifndef ABSTRACT_PROTOCOL_HPP
#define ABSTRACT_PROTOCOL_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

#include "../utils/crc.hpp"
#include "../utils/cobs.hpp"
#include "../security/aes_encryption.hpp"

class AbstractProtocol {
    private:
        EncryptionHandler encryptionHandler;
        
    protected:
        void constructPacket(const std::vector<uint8_t>& data, std::vector<uint8_t>& packet);

    public:
        bool deconstructPacket(const std::vector<uint8_t>& packet, std::vector<uint8_t>& data);

        void createRejectionPacket(std::vector<uint8_t>& packet);

        AbstractProtocol(std::vector<uint8_t>& key) : encryptionHandler(key) {}
        virtual ~AbstractProtocol() = 0;
};

#endif