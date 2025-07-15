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
		static SSL_CTX* s_server_ctx;
		static SSL_CTX* s_client_ctx;

		static const std::array<unsigned char, 4> s_cache_id;

	public:
		static int ServerInit()
		{
			OpenSSL_add_all_algorithms();
			SSL_load_error_strings();
			SSL_library_init();

			// Create Context
			s_server_ctx = SSL_CTX_new(TLS_server_method());

			if (s_server_ctx == NULL)
			{
				LOG_ERROR("OpenSSLManager: Could not create anew CTX");
				return 1;
			}

			if (!SSL_CTX_set_min_proto_version(s_server_ctx, TLS1_3_VERSION))
			{
				SSL_CTX_free(s_server_ctx);
				LOG_ERROR("OpenSSLManager: failed to set the minimum TLS protocol version.");
				return 2;
			}

			long opts;

			opts = SSL_OP_IGNORE_UNEXPECTED_EOF;
			opts |= SSL_OP_NO_RENEGOTIATION;
			opts |= SSL_OP_CIPHER_SERVER_PREFERENCE;

			SSL_CTX_set_options(s_server_ctx, opts);

			// Set certificate and private key
			if (SSL_CTX_use_certificate_file(s_server_ctx, "server.pem", SSL_FILETYPE_PEM) <= 0)
			{
				SSL_CTX_free(s_server_ctx);
				LOG_ERROR("OpenSSLManager: failed to load the server certificate.");
				return 3;
			}

			if (SSL_CTX_use_PrivateKey_file(s_server_ctx, "pkey.pem", SSL_FILETYPE_PEM) <= 0)
			{
				SSL_CTX_free(s_server_ctx);
				LOG_ERROR("OpenSSLManager: failed to load the server private key file. Possible key/cert mismatch.");
				return 4;
			}

			// Set cache mode
			SSL_CTX_set_session_id_context(s_server_ctx, OpenSSLManager::s_cache_id.data(), OpenSSLManager::s_cache_id.size());
			SSL_CTX_set_session_cache_mode(s_server_ctx, SSL_SESS_CACHE_SERVER);
			SSL_CTX_sess_set_cache_size(s_server_ctx, 1024);
			SSL_CTX_set_timeout(s_server_ctx, 3600);

			// We don't need to verify client's certificates
			SSL_CTX_set_verify(s_server_ctx, SSL_VERIFY_NONE, NULL);

			LOG_OK("OpenSSLManager: Initialization Completed!");
			return 0;
		}

		static int ClientInit()
		{
			// Create Context
			s_client_ctx = SSL_CTX_new(TLS_client_method());

			if (s_client_ctx == NULL)
			{
				LOG_ERROR("OpenSSLManager: Could not create a new CTX.");
				return 1;
			}

			// Set verify of the certs
			SSL_CTX_load_verify_locations(s_client_ctx, "server.pem", nullptr); // Trust this cert
			SSL_CTX_set_verify(s_client_ctx, SSL_VERIFY_PEER, NULL);

			// Use the default trusted certificate store
			if (!SSL_CTX_set_default_verify_paths(s_client_ctx))
			{
				LOG_ERROR("OpenSSLManager: Could not set default verify paths.");
				return 2;
			}

			// Restrict to TLS v1.3
			if (!SSL_CTX_set_min_proto_version(s_client_ctx, TLS1_3_VERSION))
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
			SSL* s = SSL_new(s_server_ctx);

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
			SSL* s = SSL_new(s_client_ctx);

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
			SSL_CTX_free(s_server_ctx);
			return 0;
		}
	};

}

#endif
