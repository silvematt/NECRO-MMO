#include "NECROWorld.h"

#include "SocketUtility.h"
#include "TCPSocket.h"
#include <memory>

namespace NECRO
{
namespace World
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

		m_configSettings.MANAGER_SERVER_PORT = conf.GetInt("MANAGER_SERVER_PORT", 61532);
		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", 1);
		m_configSettings.ASIO_THREADS_COUNT = conf.GetInt("ASIO_THREADS_COUNT", 1);
		m_configSettings.MAX_CONNECTED_CLIENTS_PER_THREAD = conf.GetInt("MAX_CONNECTED_CLIENTS_PER_THREAD", -1);
		m_configSettings.CONNECTED_AND_IDLE_TIMEOUT_MS = conf.GetInt("CONNECTED_AND_IDLE_TIMEOUT_MS", 300000);
		m_configSettings.LOGIN_DATABASE_THREADS_COUNT = conf.GetInt("LOGIN_DATABASE_THREADS_COUNT", 1);
		m_configSettings.CHARACTERS_DATABASE_THREADS_COUNT = conf.GetInt("CHARACTERS_DATABASE_THREADS_COUNT", 1);

		// Spam prevention
		m_configSettings.ENABLE_SPAM_PREVENTION = conf.GetInt("ENABLE_SPAM_PREVENTION", 1);
		m_configSettings.CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = conf.GetInt("CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN", 1);
		m_configSettings.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL = conf.GetInt("MAX_CONNECTION_ATTEMPTS_PER_INTERVAL", 10);

		m_configSettings.LOGIN_DATABASE_URI = conf.GetString("LOGIN_DATABASE_URI", "");
		m_configSettings.CHARACTERS_DATABASE_URI = conf.GetString("CHARACTERS_DATABASE_URI", "");
	}

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

		// Make DBWorker Pool for Login database
		int dbLoginThreadsCount = std::thread::hardware_concurrency();
		if (m_configSettings.LOGIN_DATABASE_THREADS_COUNT != -1)
			dbLoginThreadsCount = m_configSettings.LOGIN_DATABASE_THREADS_COUNT;

		if (dbLoginThreadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -2;
		}

		// Init DBWorkers (pools?)
		if (m_loginDbPool.Setup(dbLoginThreadsCount, m_configSettings.LOGIN_DATABASE_URI) != 0)
		{
			LOG_ERROR("Could not initialize dbworker, MySQL may be not running.");
			return -3;
		}

		if (m_loginDbPool.Start() != 0)
		{
			LOG_ERROR("Could not start LoginDBWorker, MySQL may be not running.");
			return -4;
		}
		LOG_OK("Login DBWorkerPool started successfully! {} threads.", dbLoginThreadsCount);

		// Make DBWorker Pool for Login database
		int dbCharactersThreadsCount = std::thread::hardware_concurrency();
		if (m_configSettings.CHARACTERS_DATABASE_THREADS_COUNT != -1)
			dbCharactersThreadsCount = m_configSettings.CHARACTERS_DATABASE_THREADS_COUNT;

		if (dbCharactersThreadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -5;
		}

		if (m_charactersDBPool.Setup(dbCharactersThreadsCount, m_configSettings.CHARACTERS_DATABASE_URI) != 0)
		{
			LOG_ERROR("Could not initialize CharactersDBWorker, MySQL may be not running.");
			return -6;
		}

		if (m_charactersDBPool.Start() != 0)
		{
			LOG_ERROR("Could not start dbworker, MySQL may be not running.");
			return -7;
		}
		LOG_OK("Characters DBWorker started successfully! {} threads.", dbCharactersThreadsCount);

		// Load NDBs
		int ndbsLoadReturnVal = m_ndbs.LoadFromDefinition();

		if (ndbsLoadReturnVal == 0)
		{
			LOG_ERROR("m_ndbs.LoadFromDefinition returned 0!");
			return -8;
		}
		else
			LOG_OK("Loaded {} NDBs!", ndbsLoadReturnVal);

		// After loading the NDBs, we can load the DataStores and have the fixed, contract-based agreement between game code and DB data
		// TODO: NDB in memory footprint (even if it's just a spike at load) can become huge for very big databases (like a full fledged MMORPG items db). A better way to load the stores would be to avoid loading the whole NDB in memory first
		// and just do: LoadOneRow -> LoadOneStoreDef -> DiscardTheNDBRow
		int ndbsStoresReturnVal = m_dataStores.LoadAll(m_ndbs);
		if (ndbsStoresReturnVal == 0)
		{
			LOG_ERROR("m_dataStores.LoadAll returned false! NDB: 'maps_db' could not be loaded.");
			return -9;
		}
		else
			LOG_OK("Loaded {} NDBDataStores!", ndbsStoresReturnVal);

		// We can unload the NDBs, we don't need them anymroe
		m_ndbs.Clear();
		LOG_OK("Cleared {} NDBs from memory!", ndbsLoadReturnVal);

		// Start network threads
		int threadsCount = std::thread::hardware_concurrency();

		// Make Socket Manager
		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			threadsCount = m_configSettings.NETWORK_THREADS_COUNT;
		
		if (threadsCount <= 0) // std::thread::hardware_concurrency can return 0 if it's not-computable, cover misconfig as well
		{
			LOG_WARNING("While making SocketManager, std::thread::hardware_concurrency could not be computed! Explicit NETWORK_THREADS_COUNT in the config file.");
			return -10;
		}

		m_socketManager = std::make_unique<SocketManager>(threadsCount, m_asioPool.m_ioContext, m_configSettings.MANAGER_SERVER_PORT);

		return 0;
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

		LOG_OK("NECROWorld has started.");
	}

	void Server::Update()
	{
		LOG_INFO("Starting up world simulation...");
		m_worldSimulation.Start();

		LOG_OK("NECROWorld is running!");
		while (m_worldSimulation.m_isRunning)
			m_worldSimulation.Update();

		// Here if somebody called Server::Stop()
		Shutdown();
	}

	void Server::Stop()
	{
		LOG_OK("Stopping NECROWorld...");
		m_worldSimulation.Stop();

		m_isRunning = false;
	}

	int Server::Shutdown()
	{
		// Shutdown
		LOG_OK("Shutting down NECROWorld...");

		m_asioPool.Stop();

		// Shutdown DBWorkers
		m_loginDbPool.Stop();
		m_loginDbPool.Join();
		m_loginDbPool.CloseDBs();

		m_charactersDBPool.Stop();
		m_charactersDBPool.Join();
		m_charactersDBPool.CloseDBs();

		// Shutdown Network Threads
		m_socketManager->StopThreads();
		m_socketManager->JoinThreads();

		// Orders the shutdown and calls the destructors of socketManager's members
		m_socketManager.reset();

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

		// Enqueue a keep alive packet
		DBRequest loginKeepAlivePacket(m_asioPool.m_ioContext, true);
		loginKeepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDbPool.EnqueueInAll(std::move(loginKeepAlivePacket));

		// Keep alive the direct connection as well
		DBRequest loginDirectKeepAlivePacket(m_asioPool.m_ioContext, true);
		loginDirectKeepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDbPool.DirectExecuteInAll(std::move(loginDirectKeepAlivePacket));

		// Enqueue a keep alive packet
		DBRequest charactersKeepAlivePacket(m_asioPool.m_ioContext, true);
		charactersKeepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::KEEP_ALIVE), {} });
		m_charactersDBPool.EnqueueInAll(std::move(charactersKeepAlivePacket));

		// Keep alive the direct connection as well
		DBRequest charactersDirectKeepAlivePacket(m_asioPool.m_ioContext, true);
		charactersDirectKeepAlivePacket.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::KEEP_ALIVE), {} });
		m_charactersDBPool.DirectExecuteInAll(std::move(charactersDirectKeepAlivePacket));
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
