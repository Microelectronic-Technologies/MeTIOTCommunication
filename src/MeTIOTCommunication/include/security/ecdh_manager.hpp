#ifndef ECDH_MANAGER_HPP
#define ECDH_MANAGER_HPP

#include <iostream>
#include <vector>
#include <cstdint>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/crypto.h>



class ECDHManager {
    private:
        std::vector<uint8_t> sharedSecret;
        EVP_PKEY* privateKey = nullptr;

        void handleOpenSSLError(const char* func) const;

    public:
        std::vector<uint8_t> publicKey;

        ECDHManager();

        ~ECDHManager();

        bool deriveSharedSecret(const std::vector<uint8_t>& peerPublicKey);

        const std::vector<uint8_t>& getSharedSecret() const { return sharedSecret; }
};

#endif