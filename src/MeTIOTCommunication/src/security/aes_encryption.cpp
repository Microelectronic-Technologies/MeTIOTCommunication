#include "../../include/security/aes_encryption.hpp"

void EncryptionHandler::generateIV(std::vector<uint8_t>& buffer) {
    // Generate 16 byte IV
}

EncryptionHandler::EncryptionHandler(const std::vector<uint8_t>& key) : privateKey(key) {
    // init EVP
}

EncryptionHandler::~EncryptionHandler() {
    // Write over privateKey to ensure it doesn't stay in memory
    OPENSSL_cleanse(privateKey.data(), privateKey.size());
}

void EncryptionHandler::encryptData(const std::vector<uint8_t>& data, std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& ivBuffer) {
    // Gen 16 Byte IV
    generateIV(ivBuffer);

    // Encrypt Data into Buffer

}

bool EncryptionHandler::decryptData(const std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& data, std::vector<uint8_t>& IV) {
    // Decrypt encryptedData into data
}