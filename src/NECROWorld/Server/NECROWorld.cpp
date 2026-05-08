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
			LOG_ERROR("Could not start dbworker, MySQL may be not running.");
			return -3;
		}

		// Start network threads
		int threadsCount = std::thread::hardware_concurrency();

		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			threadsCount = m_configSettings.NETWORK_THREADS_COUNT;

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
		m_configSettings.DATABASE_CALLBACK_CHECK_INTERVAL_MS = conf.GetInt("DATABASE_CALLBACK_CHECK_INTERVAL_MS", 1000);

		m_configSettings.MANAGER_SERVER_PORT = conf.GetInt("MANAGER_SERVER_PORT", 61532);
		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", 1);
		m_configSettings.ASIO_THREADS_COUNT = conf.GetInt("ASIO_THREADS_COUNT", 1);
		m_configSettings.MAX_CONNECTED_CLIENTS_PER_THREAD = conf.GetInt("MAX_CONNECTED_CLIENTS_PER_THREAD", -1);
		m_configSettings.CONNECTED_AND_IDLE_TIMEOUT_MS = conf.GetInt("CONNECTED_AND_IDLE_TIMEOUT_MS", 10000);

		// Spam prevention
		m_configSettings.ENABLE_SPAM_PREVENTION = conf.GetInt("ENABLE_SPAM_PREVENTION", 1);
		m_configSettings.CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = conf.GetInt("CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN", 1);
		m_configSettings.MAX_CONNECTION_ATTEMPTS_PER_MINUTE = conf.GetInt("MAX_CONNECTION_ATTEMPTS_PER_MINUTE", 10);

		m_configSettings.LOGIN_DATABASE_URI = conf.GetString("LOGIN_DATABASE_URI", "");
		m_configSettings.SESSIONS_DATABASE_URI = conf.GetString("SESSIONS_DATABASE_URI", "");
	}

	void Server::Start()
	{
		// Start ASIO threads
		m_asioPool.Start(m_configSettings.ASIO_THREADS_COUNT);

		// Post work on ASIO threads
		m_asioPool.PostWork([this]() {KeepDatabasesAliveHandler(); });
		m_asioPool.PostWork([this]() {LoginDBCallbackCheckHandler(); });

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

		m_loginDbWorker.Stop();
		m_loginDbWorker.Join();
		m_loginDbWorker.CloseDB();

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
		DBRequest req(m_asioPool.m_ioContext, true);
		req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDbWorker.Enqueue(std::move(req));
	}

	void Server::LoginDBCallbackCheckHandler()
	{
		//LOG_DEBUG("LoginDBCallbackCheckHandler...");

		// Calls itself again
		m_dbCallbackCheckTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_CALLBACK_CHECK_INTERVAL_MS));
		m_dbCallbackCheckTimer.async_wait([this](boost::system::error_code const& ec) { LoginDBCallbackCheckHandler(); });

		// Execute the callbacks
		std::vector<DBRequest> requests = m_loginDbWorker.GetResponseQueue();

		// Callbacks are executed on the NetworkThread's io_context associated with the WorldSocket that created the DBRequest originally, so there's no risk of race conditions
		for (auto& req : requests)
		{
			std::shared_ptr reqPtr = std::make_shared<DBRequest>(std::move(req));

			boost::asio::post(reqPtr->m_callbackContexRef, [reqPtr]()
			{
				if (reqPtr->m_callback)
					reqPtr->m_callback(reqPtr->m_errorCode, reqPtr->m_sqlResults);
			});
		}
	}
}
}
