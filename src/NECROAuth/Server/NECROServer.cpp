#include "NECROServer.h"

#include "SocketUtility.h"
#include "OpenSSLManager.h"
#include "SocketManager.h"
#include "Packet.h"

#include <memory>
#include <openssl/ssl.h>


// -------------------------------------------------------------------------------------------------------------------------------------------------
// NECROAuthentication
// 
// The way the authentication works is the following: to authenticate, users will connect to the NECROAuth Server, providing login, password, and 
// other info such as client version. The connection to the NECROAuth server will be done via TLS. When connection is granted, a sessionKey is generated 
// and put inside the database. The client will securely receive the sessionKey as well (while still being in TLS), with a 'greetCode' that will be used
// to connect with the game server. Finally, the client receives the realmlist with the active servers addresses.
// 
// The game server will NOT use TLS, instead, packets will be encrypted/decrypted by each end using the sessionKey, hashed with some random
// bytes client and server will send to each other. Once they both have their shared secrets, world packets will be encrypted via AES with that 
// encryption key. In this way we can make sure the Server receives packets from the actual authorized user, and users are able to only get and read
// packets that are destinated to them. For the first connection, the 'greetCode' estabilished during authentication is used.
// 
// The world server receives the first packet as [GREETCODE | ENCRYPTED_PACKET]. Using the GREETCODE the world server can retrieve the sessionKey used to
// decrypt the packet and start the communication.
// -------------------------------------------------------------------------------------------------------------------------------------------------

namespace NECRO
{
namespace Auth
{
	void Server::ApplySettings()
	{
		auto& conf = Config::Instance();

		// Apply config
		ConsoleLogger::Instance().m_logEnabled = conf.GetBool("ConsoleLoggingEnabled", true);
		FileLogger::Instance().m_logEnabled = conf.GetBool("FileLoggingEnabled", true);

		// Initialize the log levels
		ConsoleLogger::Instance().m_logEnabledSet = std::bitset<static_cast<int>(Logger::LogLevel::LAST_VALUE)>(conf.GetString("ConsoleLoggingLevel", "111111"));
		FileLogger::Instance().m_logEnabledSet = std::bitset<static_cast<int>(Logger::LogLevel::LAST_VALUE)>(conf.GetString("FileLoggingLevel", "111111"));

		m_configSettings.CLIENT_VERSION_MAJOR = conf.GetInt("CLIENT_VERSION_MAJOR", 1);
		m_configSettings.CLIENT_VERSION_MINOR = conf.GetInt("CLIENT_VERSION_MINOR", 0);
		m_configSettings.CLIENT_VERSION_REVISION = conf.GetInt("CLIENT_VERSION_REVISION", 0);

		m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS = conf.GetInt("DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS", 60000);
		m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS = conf.GetInt("IP_BASED_REQUEST_CLEANUP_INTERVAL_MS", 120000);

		m_configSettings.MAX_CONNECTED_CLIENTS_PER_THREAD = conf.GetInt("MAX_CONNECTED_CLIENTS_PER_THREAD", -1);
		m_configSettings.MANAGER_SERVER_PORT = conf.GetInt("MANAGER_SERVER_PORT", 61531);
		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", -1);
		m_configSettings.CRYPTO_THREADS_COUNT = conf.GetInt("CRYPTO_THREADS_COUNT", 1);
		m_configSettings.LOGIN_DATABASE_THREADS_COUNT = conf.GetInt("LOGIN_DATABASE_THREADS_COUNT", 1);

		// Spam prevention
		m_configSettings.ENABLE_SPAM_PREVENTION = conf.GetInt("ENABLE_SPAM_PREVENTION", 1);
		m_configSettings.CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = conf.GetInt("CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN", 1);
		m_configSettings.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL = conf.GetInt("MAX_CONNECTION_ATTEMPTS_PER_INTERVAL", 10);

		m_configSettings.CONNECTED_AND_IDLE_TIMEOUT_MS = conf.GetInt("CONNECTED_AND_IDLE_TIMEOUT_MS", 10000);
		m_configSettings.HANDSHAKING_AND_IDLE_TIMEOUT_MS = conf.GetInt("HANDSHAKING_AND_IDLE_TIMEOUT_MS", 10000);

		// Realmlist
		m_configSettings.REALMLIST_UPDATE_INTERVAL_MS = conf.GetInt("REALMLIST_UPDATE_INTERVAL_MS", 60000);

		// DB Connection
		m_configSettings.LOGIN_DATABASE_URI = conf.GetString("LOGIN_DATABASE_URI", "");
	}

	int Server::Init()
	{
		m_isRunning = false;

		LOG_OK("Booting up NECROAuth...");

		// Load libraries
		if (sodium_init() < 0) 
		{
			LOG_ERROR("Failed to Init Sodium.");
			return -1;
		}

		// Load config file
		if (!Config::Instance().Load(AUTH_CONFIG_FILE_PATH))
		{
			LOG_ERROR("Failed to load config file at: {}", AUTH_CONFIG_FILE_PATH);

			return -2;
		}
		LOG_OK("Config file {} loaded successfully", AUTH_CONFIG_FILE_PATH);

		// Load RealmList
		if (RealmList::Instance().Init() != 0)
		{
			return -3;
		}
		LOG_OK("RealmList object initialized.");


		// Apply Server Settings
		ApplySettings();

		SocketUtility::Initialize();

		if (OpenSSLManager::ServerInit() != 0)
			return -4;

		// Make DBWorker Pool for Login database
		int dbLoginThreadsCount = std::thread::hardware_concurrency();
		if (m_configSettings.LOGIN_DATABASE_THREADS_COUNT != -1)
			dbLoginThreadsCount = m_configSettings.LOGIN_DATABASE_THREADS_COUNT;

		if (dbLoginThreadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -5;
		}

		if (m_loginDBPool.Setup(dbLoginThreadsCount, m_configSettings.LOGIN_DATABASE_URI) != 0)
		{
			LOG_ERROR("Could not initialize m_loginDbWorker Pool, MySQL may be not running.");
			return -6;
		}

		if (m_loginDBPool.Start() != 0)
		{
			LOG_ERROR("Could not start m_loginDbWorker Pool, MySQL may be not running.");
			return -7;
		}

		// Make SocketManager
		int networkThreadsCount = std::thread::hardware_concurrency();
		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			networkThreadsCount = m_configSettings.NETWORK_THREADS_COUNT;
		
		if (networkThreadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -8;
		}

		m_socketManager = std::make_unique<SocketManager>(networkThreadsCount, m_ioContext, m_configSettings.MANAGER_SERVER_PORT);

		return 0;
	}

	void Server::Start()
	{
		// Start CryptoThreads
		int cryptoThreadsCount = std::thread::hardware_concurrency();

		if (m_configSettings.CRYPTO_THREADS_COUNT != -1)
			cryptoThreadsCount = m_configSettings.CRYPTO_THREADS_COUNT;

		if (cryptoThreadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While starting CryptoThreads, std::thread::hardware_concurrency could not be computed! Explicit CRYPTO_THREADS_COUNT in the config file. Running 1 CryptoThread for this execution...");
			cryptoThreadsCount = 1;
		}

		m_cryptoThreads.Start(cryptoThreadsCount);

		// Post DB Handler
		m_keepLoginDatabaseAliveTimer.expires_after(std::chrono::milliseconds(1));
		m_keepLoginDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Post ip request cleanup
		m_ipRequestCleanupTimer.expires_after(std::chrono::milliseconds(1));
		m_ipRequestCleanupTimer.async_wait([this](boost::system::error_code const& ec) { IPRequestCleanupHandler(); });

		// Get realmlist straight away (DirectExecute)
		{
			DBRequest req(m_ioContext, false);
			req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::GATHER_REALMS), {} });

			std::vector<mysqlx::SqlResult> res = m_loginDBPool.DirectExecute(req);

			if (!res.empty())
				RealmList::Instance().DBCallback_UpdateRealmList(req.m_errorCode, res);
			else
			{
				LOG_ERROR("Could not gather realms.");
				Stop();
				return;
			}
		}

		// Post Realms Update
		m_realmlistUpdateTimer.expires_after(std::chrono::milliseconds(m_configSettings.REALMLIST_UPDATE_INTERVAL_MS));
		m_realmlistUpdateTimer.async_wait([this](boost::system::error_code const& ec) { UpdateRealmlistHandler(); });

		// Start network threads
		m_socketManager->StartThreads();

		// Start the socket's manager work
		m_socketManager->Start();

		m_isRunning = true;
		LOG_OK("NECROAuth is running...");
	}

	void Server::Update()
	{
		// Boost Event Loop
		m_ioContext.run();

		// Here if somebody called Server::Stop() or the m_ioContext ran out of work
		Shutdown();
	}

	void Server::Stop()
	{
		LOG_OK("Stopping NECROAuth...");

		m_isRunning = false;
		m_ioContext.stop();
	}

	int Server::Shutdown()
	{
		// Shutdown
		LOG_OK("Shutting down NECROAuth...");

		// Shutdown DBWorker
		m_loginDBPool.Stop();
		m_loginDBPool.Join();
		m_loginDBPool.CloseDBs();

		// Stop crypto threads
		m_cryptoThreads.Stop();

		// Shutdown NetworkThreads
		m_socketManager->StopThreads();
		m_socketManager->JoinThreads();

		LOG_OK("Shut down of the NECROAuth completed.");
		return 0;
	}


	void Server::KeepDatabaseAliveHandler()
	{
		//LOG_DEBUG("KeepDatabaseAliveHandler...");

		// Calls itself again
		m_keepLoginDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepLoginDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Enqueue a keep alive packet
		DBRequest keepAlivePacket(m_ioContext, true);
		keepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDBPool.EnqueueInAll(std::move(keepAlivePacket));

		// Keep alive the direct connection as well
		DBRequest directKeepAlivePacket(m_ioContext, true);
		directKeepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDBPool.DirectExecuteInAll(std::move(directKeepAlivePacket));
	}

	void Server::IPRequestCleanupHandler()
	{
		//LOG_DEBUG("IPRequestCleanupHandler...");

		m_ipRequestCleanupTimer.expires_after(std::chrono::milliseconds(m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS));
		m_ipRequestCleanupTimer.async_wait([this](boost::system::error_code const& ec) { IPRequestCleanupHandler(); });

		// Clear the ip-request map (m_socketManager is constructed on the same io_context, so no race conditions since only the main thread runs io_context.run())
		m_socketManager->IPRequestMapCleanup();
	}

	void Server::UpdateRealmlistHandler()
	{
		// LOG_DEBUG("UpdateRealmlistHandler...");

		m_realmlistUpdateTimer.expires_after(std::chrono::milliseconds(m_configSettings.REALMLIST_UPDATE_INTERVAL_MS));
		m_realmlistUpdateTimer.async_wait([this](boost::system::error_code const& ec) { UpdateRealmlistHandler(); });

		auto& dbworker = m_loginDBPool;
		// Send DB Gather Realms request
		{
			DBRequest req(m_ioContext, false);
			req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::GATHER_REALMS), {} });
			
			req.m_callback = [this](uint32_t ec, std::vector<mysqlx::SqlResult>& res)
			{
				return RealmList::Instance().DBCallback_UpdateRealmList(ec, res);
			};
			
			dbworker.Enqueue(std::move(req));
		}
	}
}
}
