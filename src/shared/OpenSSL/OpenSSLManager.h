#ifndef NECRO_OPEN_SSL_MANAGER
#define NECRO_OPEN_SSL_MANAGER

#include "ConsoleLogger.h"
#include "FileLogger.h"

#ifdef _WIN32
	#include "WinSock2.h"
	#include <WS2tcpip.h>
#endif


#include <array>
#include <openssl/ssl.h>

namespace NECRO
{
	#ifdef _WIN32
		typedef SOCKET sock_t;
	#else
		typedef int sock_t;
	#endif

	class OpenSSLManager
	{
	private:
		static SSL_CTX* server_ctx;
		static SSL_CTX* client_ctx;

		static const std::array<unsigned char, 4> cache_id;

	public:
		static int ServerInit()
		{
			OpenSSL_add_all_algorithms();
			SSL_load_error_strings();
			SSL_library_init();

			// Create Context
			server_ctx = SSL_CTX_new(TLS_server_method());

			if (server_ctx == NULL)
			{
				LOG_ERROR("OpenSSLManager: Could not create anew CTX");
				return 1;
			}

			if (!SSL_CTX_set_min_proto_version(server_ctx, TLS1_3_VERSION))
			{
				SSL_CTX_free(server_ctx);
				LOG_ERROR("OpenSSLManager: failed to set the minimum TLS protocol version.");
				return 2;
			}

			long opts;

			opts = SSL_OP_IGNORE_UNEXPECTED_EOF;
			opts |= SSL_OP_NO_RENEGOTIATION;
			opts |= SSL_OP_CIPHER_SERVER_PREFERENCE;

			SSL_CTX_set_options(server_ctx, opts);

			// Set certificate and private key
			if (SSL_CTX_use_certificate_file(server_ctx, "server.pem", SSL_FILETYPE_PEM) <= 0)
			{
				SSL_CTX_free(server_ctx);
				LOG_ERROR("OpenSSLManager: failed to load the server certificate.");
				return 3;
			}

			if (SSL_CTX_use_PrivateKey_file(server_ctx, "pkey.pem", SSL_FILETYPE_PEM) <= 0)
			{
				SSL_CTX_free(server_ctx);
				LOG_ERROR("OpenSSLManager: failed to load the server private key file. Possible key/cert mismatch.");
				return 4;
			}

			// Set cache mode
			SSL_CTX_set_session_id_context(server_ctx, OpenSSLManager::cache_id.data(), OpenSSLManager::cache_id.size());
			SSL_CTX_set_session_cache_mode(server_ctx, SSL_SESS_CACHE_SERVER);
			SSL_CTX_sess_set_cache_size(server_ctx, 1024);
			SSL_CTX_set_timeout(server_ctx, 3600);

			// We don't need to verify client's certificates
			SSL_CTX_set_verify(server_ctx, SSL_VERIFY_NONE, NULL);

			LOG_OK("OpenSSLManager: Initialization Completed!");
			return 0;
		}

		static int ClientInit()
		{
			// Create Context
			client_ctx = SSL_CTX_new(TLS_client_method());

			if (client_ctx == NULL)
			{
				LOG_ERROR("OpenSSLManager: Could not create a new CTX.");
				return 1;
			}

			// Set verify of the certs
			SSL_CTX_load_verify_locations(client_ctx, "server.pem", nullptr); // Trust this cert
			SSL_CTX_set_verify(client_ctx, SSL_VERIFY_PEER, NULL);

			// Use the default trusted certificate store
			if (!SSL_CTX_set_default_verify_paths(client_ctx))
			{
				LOG_ERROR("OpenSSLManager: Could not set default verify paths.");
				return 2;
			}

			// Restrict to TLS v1.3
			if (!SSL_CTX_set_min_proto_version(client_ctx, TLS1_3_VERSION))
			{
				LOG_ERROR("OpenSSLManager: failed to set min protocol version.");
				return 3;
			}

			LOG_OK("OpenSSLManager: Initialization Completed!");
			return 0;
		}

		static BIO* CreateBioAndWrapSocket(sock_t s)
		{
			BIO* b = BIO_new(BIO_s_socket());

			if (b == NULL)
			{
				LOG_ERROR("OpenSSLManager: failed to create a BIO socket object.");
				return nullptr;
			}

			// Wrap the socket | Socket will be closed when BIO is freed with the BIO_CLOSE option
			BIO_set_fd(b, s, BIO_CLOSE);

			return b;
		}

		static SSL* ServerCreateSSLObject(BIO* setBio = nullptr)
		{
			SSL* s = SSL_new(server_ctx);

			if (s == NULL)
			{
				LOG_ERROR("OpenSSLManager: failed to create a SSL object.");
				return nullptr;
			}

			if (setBio != nullptr)
			{
				SSL_set_bio(s, setBio, setBio);
			}

			return s;
		}

		static SSL* ClientCreateSSLObject(BIO* setBio = nullptr)
		{
			SSL* s = SSL_new(client_ctx);

			if (s == NULL)
			{
				LOG_ERROR("OpenSSLManager: failed to create a SSL object.");
				return nullptr;
			}

			if (setBio != nullptr)
			{
				SSL_set_bio(s, setBio, setBio);
			}

			return s;
		}

		static void SetSNIHostname(SSL* s, const char* hostname)
		{
			if (!SSL_set_tlsext_host_name(s, hostname))
			{
				LOG_ERROR("OpenSSLManager: failed to SetSNIHostname.");
			}
		}

		static void SetCertVerificationHostname(SSL* s, const char* hostname)
		{
			if (!SSL_set1_host(s, hostname))
			{
				LOG_ERROR("OpenSSLManager: failed to SetCertVerificationHostname.");
			}
		}

		static void SSL_SetBio(SSL* s, BIO* read, BIO* write)
		{
			SSL_set_bio(s, read, write);
		}

		static int ServerShutdown()
		{
			SSL_CTX_free(server_ctx);
			return 0;
		}
	};

}

#endif
