#include "../../include/security/ecdh_manager.hpp"

void ECDHManager::handleOpenSSLError(const char* func) const {
    std::cerr << "OpenSSL Error in " << func << ": ";
    ERR_print_errors_fp(stderr);
}

ECDHManager::ECDHManager() {
    
}