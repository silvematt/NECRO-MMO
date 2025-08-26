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
	int Server::Init()
	{
		m_isRunning = false;

		LOG_OK("Booting up NECROAuth...");

		// Load config file
		if (!Config::Instance().Load(AUTH_CONFIG_FILE_PATH))
		{
			LOG_ERROR("Failed to load config file at: {}", AUTH_CONFIG_FILE_PATH);

			return -1;
		}
		LOG_OK("Config file {} loaded successfully", AUTH_CONFIG_FILE_PATH);

		// Apply Server Settings
		ApplySettings();

		SocketUtility::Initialize();

		if (OpenSSLManager::ServerInit() != 0)
			return -1;

		/*
		if (m_directdb.Init() != 0)
		{
			LOG_ERROR("Could not initialize directdb, MySQL may be not running.");
			return -2;
		}
		*/

		if (m_dbworker.Setup(Database::DBType::LOGIN_DATABASE) != 0)
		{
			LOG_ERROR("Could not initialize directdb, MySQL may be not running.");
			return -3;
		}

		if (m_dbworker.Start() != 0)
		{
			LOG_ERROR("Could not start dbworker, MySQL may be not running.");
			return -4;
		}

		// Make TCPSocketManager
		int threadsCount = std::thread::hardware_concurrency();

		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			threadsCount = m_configSettings.NETWORK_THREADS_COUNT;

		m_socketManager = std::make_unique<SocketManager>(threadsCount, m_ioContext, m_configSettings.MANAGER_SERVER_PORT);

		return 0;
	}

	void Server::ApplySettings()
	{
		auto& conf = Config::Instance();

		// Apply config
		ConsoleLogger::Instance().m_logEnabled	= conf.GetBool("ConsoleLoggingEnabled", true);
		FileLogger::Instance().m_logEnabled		= conf.GetBool("FileLoggingEnabled", true);

		m_configSettings.CLIENT_VERSION_MAJOR		= conf.GetInt("CLIENT_VERSION_MAJOR", 1);
		m_configSettings.CLIENT_VERSION_MINOR		= conf.GetInt("CLIENT_VERSION_MINOR", 0);
		m_configSettings.CLIENT_VERSION_REVISION	= conf.GetInt("CLIENT_VERSION_REVISION", 0);

		m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS = conf.GetInt("DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS", 60000);
		m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS = conf.GetInt("IP_BASED_REQUEST_CLEANUP_INTERVAL_MS", 60000);
		m_configSettings.DATABASE_CALLBACK_CHECK_INTERVAL_MS = conf.GetInt("DATABASE_CALLBACK_CHECK_INTERVAL_MS", 1000);

		m_configSettings.MAX_CONNECTED_CLIENTS_PER_THREAD = conf.GetInt("MAX_CONNECTED_CLIENTS_PER_THREAD", -1);
		m_configSettings.MANAGER_SERVER_PORT = conf.GetInt("MANAGER_SERVER_PORT", 61531);
		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", -1);

		// Spam prevention
		m_configSettings.ENABLE_SPAM_PREVENTION = conf.GetInt("ENABLE_SPAM_PREVENTION", 1);
		m_configSettings.CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = conf.GetInt("CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN", 1);
		m_configSettings.MAX_CONNECTION_ATTEMPTS_PER_MINUTE = conf.GetInt("MAX_CONNECTION_ATTEMPTS_PER_MINUTE", 10);

		m_configSettings.CONNECTED_AND_IDLE_TIMEOUT_MS = conf.GetInt("CONNECTED_AND_IDLE_TIMEOUT_MS", 10000);
		m_configSettings.HANDSHAKING_AND_IDLE_TIMEOUT_MS = conf.GetInt("HANDSHAKING_AND_IDLE_TIMEOUT_MS", 10000);
	}

	void Server::Start()
	{
		// Post DB Handler
		m_keepDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Post ip request cleanup
		m_ipRequestCleanupTimer.expires_after(std::chrono::milliseconds(m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS));
		m_ipRequestCleanupTimer.async_wait([this](boost::system::error_code const& ec) { IPRequestCleanupHandler(); });

		// Post database callback check
		m_dbCallbackCheckTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_CALLBACK_CHECK_INTERVAL_MS));
		m_dbCallbackCheckTimer.async_wait([this](boost::system::error_code const& ec) { DBCallbackCheckHandler(); });

		// Start network threads
		m_socketManager->StartThreads();

		// Start the socket's manager work
		m_socketManager->Start();

		m_isRunning = true;
		LOG_OK("NECROAuth is running...");
	}

	void Server::Update()
	{
		// Boost Loop
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

		//m_directdb.Close();

		m_dbworker.Stop();
		m_dbworker.Join();
		m_dbworker.CloseDB();

		LOG_OK("Shut down of the NECROAuth completed.");
		return 0;
	}


	void Server::KeepDatabaseAliveHandler()
	{
		//LOG_DEBUG("KeepDatabaseAliveHandler...");

		// Calls itself again
		m_keepDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Enqueue a keep alive packet
		DBRequest req(static_cast<int>(LoginDatabaseStatements::KEEP_ALIVE), m_ioContext, true);
		m_dbworker.Enqueue(std::move(req));
	}

	void Server::IPRequestCleanupHandler()
	{
		//LOG_DEBUG("IPRequestCleanupHandler...");

		m_ipRequestCleanupTimer.expires_after(std::chrono::milliseconds(m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS));
		m_ipRequestCleanupTimer.async_wait([this](boost::system::error_code const& ec) { IPRequestCleanupHandler(); });

		// Clear the ip-request map (m_socketManager is constructed on the same io_context, so no race conditions)
		m_socketManager->IPRequestMapCleanup();
	}

	void Server::DBCallbackCheckHandler()
	{
		//LOG_DEBUG("DBCallbackCheckHandler...");

		m_dbCallbackCheckTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_CALLBACK_CHECK_INTERVAL_MS));
		m_dbCallbackCheckTimer.async_wait([this](boost::system::error_code const& ec) { DBCallbackCheckHandler(); });

		// Execute the callbacks
		std::vector<DBRequest> requests = m_dbworker.GetResponseQueue();

		// Callbacks are executed on the NetworkThread's context associated with the AuthSocket that created the DBRequest, so there's no risk of race conditions
		for (auto& req : requests)
		{
			auto reqPtr = std::make_shared<DBRequest>(std::move(req));

			boost::asio::post(req.m_callbackContexRef, [reqPtr]()
			{
				if (reqPtr->m_callback)
					reqPtr->m_callback(reqPtr->m_sqlRes);
			});
			
		}
	}
}
}
