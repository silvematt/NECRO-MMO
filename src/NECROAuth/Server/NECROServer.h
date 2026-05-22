#ifndef NECROAUTHSERVER_H
#define NECROAUTHSERVER_H

#include <boost/asio.hpp>

#include "Config.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "SocketManager.h"

#include "LoginDatabase.h"
#include "DatabaseWorkerPool.h"
#include "AsioThreadPool.h"

#include "RealmList.h"

#define SODIUM_STATIC
#include <sodium.h>

// Some thoughts
// 
// It could be beneficial to have SSLAccept not do any work at all, just distribute the socket across network threads, and let the network threads run thigs like IPMapChecks or offload that somewhere else
// Could also explore multi-accept on multiple threads
// 
// TODO: Add per account rate limit:
// If three times a password is guessed wrong, and the user "lastip" in the db is different that NULL, we flag the account and prevent new requests (either for x mins, or until real user confirms an email, see explaination)
// To prevent DoS at username level, we can do this: when a user registers his account table will have a "lastip" used.
// lastip used is updated on every successful authentication
// if the current request's ip matches the "lastip" in the DB, we don't care if the account is flagged or not
// if the account is flagged and the legit user changed his IP, we can put in place a way to clear "lastip" from the DB so the per-account-rate-limit is going to be disabled until a new successful authentication, like an email-confirmation step on the next attempt "we noticed unusual login activity, click this link"
// we could also let set lastip in the same transaction that consumes the email token, so write lastip = <IP that requested the recovery>
// 
// TODO: Add client proof of work before TLS setup
// 
// TODO: Add per ip concurrent connections limit, stricter than m_ipRequestMap so that we can make the m_ipRequestMap more generous

namespace NECRO
{
namespace Auth
{
	inline constexpr const char* AUTH_CONFIG_FILE_PATH = "authserver.conf";

	class Server
	{
	public:
		// Server settings that can be overridden by config file
		struct ConfigSettings
		{
			uint8_t CLIENT_VERSION_MAJOR = 1;
			uint8_t CLIENT_VERSION_MINOR = 0;
			uint8_t CLIENT_VERSION_REVISION = 0;

			// Handler updates
			uint32_t DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS = 60000;
			uint32_t IP_BASED_REQUEST_CLEANUP_INTERVAL_MS = 120000;

			// Server settings
			uint16_t	MANAGER_SERVER_PORT = 61531;
			int			MAX_CONNECTED_CLIENTS_PER_THREAD = -1; //-1 equals to no check
			int			NETWORK_THREADS_COUNT = -1; //-1 equals to std::thread::hardware_concurrency()
			int			CONNECTED_AND_IDLE_TIMEOUT_MS = 10000; // After CONNECTED_AND_IDLE_TIMEOUT_MS, kick the client if he doesn't proceed with the communication
			int			HANDSHAKING_AND_IDLE_TIMEOUT_MS = 10000;
			int			CRYPTO_THREADS_COUNT = 1;
			int			LOGIN_DATABASE_THREADS_COUNT = 1;

			// Spam prevention
			bool		ENABLE_SPAM_PREVENTION = 1;
			uint32_t	CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = 1;
			uint32_t	MAX_CONNECTION_ATTEMPTS_PER_INTERVAL = 10;

			// Realmlist
			uint32_t REALMLIST_UPDATE_INTERVAL_MS = 60000;

			// DB Connection
			std::string LOGIN_DATABASE_URI;
		};

	public:
		Server() :
			m_isRunning(false), m_keepLoginDatabaseAliveTimer(m_ioContext), m_ipRequestCleanupTimer(m_ioContext), m_realmlistUpdateTimer(m_ioContext)
		{
		}

		static Server& Instance()
		{
			static Server instance;
			return instance;
		}

	private:
		// AsioThreadPool owns its own m_ioContext
		// Used to compute password hashes posted by AuthSessions
		AsioThreadPool m_cryptoThreads;

		// Status
		bool			m_isRunning;
		ConfigSettings	m_configSettings;

		boost::asio::io_context m_ioContext;
		std::unique_ptr<SocketManager> m_socketManager;

		DatabaseWorkerPool<LoginDatabase> m_loginDBPool;

		// Handlers on main ioContext 
		boost::asio::steady_timer m_keepLoginDatabaseAliveTimer;
		boost::asio::steady_timer m_ipRequestCleanupTimer;
		boost::asio::steady_timer m_realmlistUpdateTimer;

		void KeepDatabaseAliveHandler();
		void IPRequestCleanupHandler();
		void UpdateRealmlistHandler();

	public:
		DatabaseWorkerPool<LoginDatabase>& GetLoginDBWPool()
		{
			return m_loginDBPool;
		}

		int						Init();
		void					ApplySettings();
		void					Start();
		void					Update();
		void					Stop();
		int						Shutdown();


		const ConfigSettings& GetSettings() const
		{
			return m_configSettings;
		}

		AsioThreadPool& GetCryptoThreads()
		{
			return m_cryptoThreads;
		}
	};
}
}

#endif
