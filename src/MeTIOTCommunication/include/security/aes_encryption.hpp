#ifndef AES_ENCRYPTION_HPP
#define AES_ENCRYPTION_HPP

#include <vector>
#include <cstdint>
#include <random>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/crypto.h>

#define IV_SIZE 16

class EncryptionHandler {
    private:
        std::vector<uint8_t> privateKey;

        /* @brief   Generates 16 byte IV into buffer
         *
         * @param   Buffer   Where the IV will be stored after generation
         * 
         * @note    Implementation uses random_device. The crypto-security of this implementation is dependent on the device.
         */
        void generateIV(std::vector<uint8_t>& buffer);

    public:
        EncryptionHandler(const std::vector<uint8_t>& key) : privateKey(key) {};
        ~EncryptionHandler();

        /* @brief   Generates IV & encryptes data
         *
         * @param   data            Data that is being encrypted
         * @param   encryptedData   The encrypted data generated within this function will be put in here
         * @param   ivBuffer        The IV generated within this function will be put in here (so you can append it to the message)
         */
        void encryptData(std::vector<uint8_t>& data, std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& ivBuffer);
        
        /* @brief    Decrypts data using supplied IV
         *
         * @param    encryptedData   Encrypted payload (AES-256-CBC) to be decrypted
         * @param    data            The decrypted data generated within this function will be put here
         * @param    IV              IV used for the encrypted data
         */
        void decryptData(const std::vector<uint8_t>& encryptedData, std::vector<uint8_t>& data, const std::vector<uint8_t>& IV);

};

#endif