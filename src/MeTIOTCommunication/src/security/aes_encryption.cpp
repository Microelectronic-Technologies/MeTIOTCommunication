#include "../../include/security/aes_encryption.hpp"

void EncryptionHandler::generateIV(std::vector<uint8_t>& buffer) {
    // Generate 16 byte IV
    std::random_device rd;
    buffer.resize(IV_SIZE);
    
    std::generate(buffer.begin(), buffer.end(), [&rd]() {
        return static_cast<uint8_t>(rd());
    });
}

EncryptionHandler::~EncryptionHandler() {
    // Write over privateKey to ensure it doesn't stay in memory
    OPENSSL_cleanse(privateKey.data(), privateKey.size());
}

void EncryptionHandler::encryptData(std::vector<uint8_t>& data, std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& ivBuffer) {
    // Gen 16 Byte IV
    generateIV(ivBuffer);

    // Encrypt Data into Buffer
    EVP_CIPHER_CTX *ctx = nullptr;
    
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        // TODO: Handle error
        std::cout << "ctx fail" << std::endl;
        return;
    }

    // Init encryption operation
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, privateKey.data(), ivBuffer.data())) {
        // TODO: Handle error
        std::cout << "Init fail" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    // Provide message to be encrypted
    int dataLen = data.size();
    int updateLen = 0; // Length of data written by EVP_EncryptUpdate
    int finalLen = 0;  // Length of data written by EVP_EncryptFinal_ex
    int totalEncryptedLen = 0;

    encryptedData.resize(dataLen + 16); // Maximum possible size

    // Provide message being encrypted
    if (1 != EVP_EncryptUpdate(ctx, encryptedData.data(), &updateLen, data.data(), dataLen)) {
        // TODO: Handle error
        std::cout << "Update fail" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        encryptedData.clear();
        return;
    }

    totalEncryptedLen = updateLen;

    // Finalize the encryption
    if (1 != EVP_EncryptFinal_ex(ctx, encryptedData.data() + totalEncryptedLen, &finalLen)) {
        // TODO: Handle error
        std::cout << "Final fail" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        encryptedData.clear();
        return; 
    }

    totalEncryptedLen += finalLen;

    encryptedData.resize(totalEncryptedLen);

    EVP_CIPHER_CTX_free(ctx);
}

void EncryptionHandler::decryptData(const std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& data, const std::vector<uint8_t>& IV) {
    // Decrypt encryptedData into data
    EVP_CIPHER_CTX* ctx = nullptr;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        // TODO: Handle error
        std::cout << "ctx fail" << std::endl;
        return;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, privateKey.data(), IV.data())) {
        // TODO: Handle error
        std::cout << "DecryptInit fail" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return;
    }

    int encryptedLen = encryptedData.size();
    int updateLen = 0;
    int finalLen = 0;
    int totalDecryptedLen = 0;

    data.resize(encryptedLen);

    if (1 != EVP_DecryptUpdate(ctx, data.data(), &updateLen, encryptedData.data(), encryptedLen)) {
        // TODO: Handle error (Bad key/IV)
        std::cout << "Bad key/IV" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        data.clear();
        return;
    }

    totalDecryptedLen = updateLen;

    if (1 != EVP_DecryptFinal_ex(ctx, data.data() + totalDecryptedLen, &finalLen)) {
        // TODO: Handle error (Auth/Padding error)
        std::cout << "Auth/Padding error" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        data.clear();
        return;
    }

    totalDecryptedLen += finalLen;

    data.resize(totalDecryptedLen);

    EVP_CIPHER_CTX_free(ctx);
}