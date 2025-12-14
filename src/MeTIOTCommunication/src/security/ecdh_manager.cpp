#include "../../include/security/ecdh_manager.hpp"

void ECDHManager::handleOpenSSLError(const char* func) const {
    std::cerr << "OpenSSL Error in " << func << ": ";
    ERR_print_errors_fp(stderr);
}

ECDHManager::ECDHManager() {
    // Gen private key then public key
}

ECDHManager::~ECDHManager() {
    // Delete sharedSecret & privateKey securely
}

std::vector<uint8_t> ECDHManager::generateECDHPacket() {
    // -- CRC
    // Gen

    // Add to buffer

    // -- COBS
    
}

bool ECDHManager::deriveSharedSecret(const std::vector<uint8_t>& peerPublicKey) {

}