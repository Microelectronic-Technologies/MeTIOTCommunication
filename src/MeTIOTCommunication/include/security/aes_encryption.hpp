#ifndef AES_ENCRYPTION_HPP
#define AES_ENCRYPTION_HPP

#include <vector>
#include <cstdint>
#include <openssl/evp.h>
#include <openssl/crypto.h>

class EncryptionHandler {
    private:
        std::vector<uint8_t> privateKey;

        void generateIV(std::vector<uint8_t>& buffer);

    public:
        EncryptionHandler(const std::vector<uint8_t>& key);
        ~EncryptionHandler();

        void encryptData(const std::vector<uint8_t>& data, std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& ivBuffer);
        bool decryptData(const std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& data, std::vector<uint8_t>& IV);

};

#endif