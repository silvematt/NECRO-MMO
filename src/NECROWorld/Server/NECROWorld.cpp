#include "NECROWorld.h"

#include "SocketUtility.h"
#include "TCPSocket.h"
#include <memory>

namespace NECRO
{
namespace World
{
	int Server::Init()
	{
		m_isRunning = false;

		LOG_OK("Booting up NECROServer...");

		// Load config file
		if (!Config::Instance().Load(WORLD_CONFIG_FILE_PATH))
		{
			LOG_ERROR("Failed to load config file at: {}", WORLD_CONFIG_FILE_PATH);

			return -1;
		}
		LOG_OK("Config file {} loaded successfully", WORLD_CONFIG_FILE_PATH);

		ApplySettings();

		SocketUtility::Initialize();

		// Init DBWorkers (pools?)
		if (m_loginDbWorker.Setup(m_configSettings.LOGIN_DATABASE_URI) != 0)
		{
			LOG_ERROR("Could not initialize dbworker, MySQL may be not running.");
			return -2;
		}

		if (m_loginDbWorker.Start() != 0)
		{
			LOG_ERROR("Could not start LoginDBWorker, MySQL may be not running.");
			return -3;
		}
		LOG_OK("Login DBWorker started successfully!");

		if (m_charactersDBWorker.Setup(m_configSettings.CHARACTERS_DATABASE_URI) != 0)
		{
			LOG_ERROR("Could not initialize CharactersDBWorker, MySQL may be not running.");
			return -4;
		}

		if (m_charactersDBWorker.Start() != 0)
		{
			LOG_ERROR("Could not start dbworker, MySQL may be not running.");
			return -5;
		}
		LOG_OK("Characters DBWorker started successfully!");

		// Start network threads
		int threadsCount = std::thread::hardware_concurrency();

		// Make Socket Manager
		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			threadsCount = m_configSettings.NETWORK_THREADS_COUNT;
		
		if (threadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -4;
		}

		m_socketManager = std::make_unique<SocketManager>(threadsCount, m_asioPool.m_ioContext, m_configSettings.MANAGER_SERVER_PORT);

		return 0;
	}

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

		m_configSettings.MANAGER_SERVER_PORT = conf.GetInt("MANAGER_SERVER_PORT", 61532);
		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", 1);
		m_configSettings.ASIO_THREADS_COUNT = conf.GetInt("ASIO_THREADS_COUNT", 1);
		m_configSettings.MAX_CONNECTED_CLIENTS_PER_THREAD = conf.GetInt("MAX_CONNECTED_CLIENTS_PER_THREAD", -1);
		m_configSettings.CONNECTED_AND_IDLE_TIMEOUT_MS = conf.GetInt("CONNECTED_AND_IDLE_TIMEOUT_MS", 10000);

		// Spam prevention
		m_configSettings.ENABLE_SPAM_PREVENTION = conf.GetInt("ENABLE_SPAM_PREVENTION", 1);
		m_configSettings.CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = conf.GetInt("CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN", 1);
		m_configSettings.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL = conf.GetInt("MAX_CONNECTION_ATTEMPTS_PER_INTERVAL", 10);

		m_configSettings.LOGIN_DATABASE_URI = conf.GetString("LOGIN_DATABASE_URI", "");
		m_configSettings.CHARACTERS_DATABASE_URI = conf.GetString("CHARACTERS_DATABASE_URI", "");
	}

	void Server::Start()
	{
		// Start ASIO threads
		int asioThreadCount = std::thread::hardware_concurrency();

		if (m_configSettings.ASIO_THREADS_COUNT != -1)
			asioThreadCount = m_configSettings.ASIO_THREADS_COUNT;

		if (asioThreadCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While starting AsioThreads, std::thread::hardware_concurrency could not be computed! Explicit ASIO_THREADS_COUNT in the config file. Running 1 AsioThread for this execution...");
			asioThreadCount = 1;
		}
		m_asioPool.Start(asioThreadCount);

		// Post work on ASIO threads
		m_asioPool.PostWork([this]() {KeepDatabasesAliveHandler(); });
		m_asioPool.PostWork([this]() {IPRequestMapCleanupHandler(); });

		// Start network threads
		m_socketManager->StartThreads();

		// Start the socket's manager work
		m_socketManager->Start();

		m_isRunning = true;
		LOG_OK("NECROWorld is running...");
	}

	void Server::Update()
	{
		while (1)
		{

		}

		// Here if somebody called Server::Stop()
		Shutdown();
	}

	void Server::Stop()
	{
		LOG_OK("Stopping NECROWorld...");

		m_isRunning = false;
	}

	int Server::Shutdown()
	{
		// Shutdown
		LOG_OK("Shutting down NECROWorld...");

		m_asioPool.Stop();

		// Shutdown DBWorkers
		m_loginDbWorker.Stop();
		m_loginDbWorker.Join();
		m_loginDbWorker.CloseDB();

		m_charactersDBWorker.Stop();
		m_charactersDBWorker.Join();
		m_charactersDBWorker.CloseDB();

		// Shutdown Network Threads
		m_socketManager->StopThreads();
		m_socketManager->JoinThreads();

		LOG_OK("Shut down of NECROWorld completed.");

		return 0;
	}

	// Asio
	void Server::KeepDatabasesAliveHandler()
	{
		LOG_DEBUG("KeepDatabasesAliveHandler...");

		// Calls itself again
		m_keepLoginDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepLoginDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabasesAliveHandler(); });

		// Enqueue a keep alive packet (LoginDatabase)
		DBRequest logReq(m_asioPool.m_ioContext, true);
		logReq.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDbWorker.Enqueue(std::move(logReq));

		// Enqueue a keep alive packet (CharactersDatabase)
		DBRequest charReq(m_asioPool.m_ioContext, true);
		charReq.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::KEEP_ALIVE), {} });
		m_charactersDBWorker.Enqueue(std::move(charReq));
	}


	void Server::IPRequestMapCleanupHandler()
	{
		//LOG_DEBUG("IPRequestCleanupHandler...");

		m_ipRequestCleanupTimer.expires_after(std::chrono::milliseconds(m_configSettings.IP_BASED_REQUEST_CLEANUP_INTERVAL_MS));
		m_ipRequestCleanupTimer.async_wait([this](boost::system::error_code const& ec) { IPRequestMapCleanupHandler(); });

		m_socketManager->IPRequestMapCleanup();
	}
}
}
