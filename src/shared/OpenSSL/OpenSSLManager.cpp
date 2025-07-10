#include "OpenSSLManager.h"
// Static member definitions
SSL_CTX* OpenSSLManager::server_ctx = nullptr;
SSL_CTX* OpenSSLManager::client_ctx = nullptr;
const std::array<unsigned char, 4> OpenSSLManager::cache_id = { {0xDE, 0xAD, 0xBE, 0xEF} };

