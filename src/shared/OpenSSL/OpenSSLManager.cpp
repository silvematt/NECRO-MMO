#include "OpenSSLManager.h"

namespace NECRO
{
	// Static member definitions
	SSL_CTX* OpenSSLManager::s_server_ctx = nullptr;
	SSL_CTX* OpenSSLManager::s_client_ctx = nullptr;
	const std::array<unsigned char, 4> OpenSSLManager::s_cache_id = { {0xDE, 0xAD, 0xBE, 0xEF} };
}
