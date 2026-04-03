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

		int threadsCount = std::thread::hardware_concurrency();

		if (m_configSettings.NETWORK_THREADS_COUNT != -1)
			threadsCount = m_configSettings.NETWORK_THREADS_COUNT;

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

		m_configSettings.NETWORK_THREADS_COUNT = conf.GetInt("NETWORK_THREADS_COUNT", -1);

		m_configSettings.LOGIN_DATABASE_URI = conf.GetString("LOGIN_DATABASE_URI", "");
		m_configSettings.SESSIONS_DATABASE_URI = conf.GetString("SESSIONS_DATABASE_URI", "");
	}

	void Server::Start()
	{
		// Post DB Handler
		m_keepLoginDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepLoginDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Start network threads

		m_isRunning = true;
		LOG_OK("NECROWorld is running...");
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
		LOG_OK("Stopping NECROWorld...");

		m_isRunning = false;
		m_ioContext.stop();
	}

	int Server::Shutdown()
	{
		// Shutdown
		LOG_OK("Shutting down NECROWorld...");

		m_loginDbWorker.Stop();
		m_loginDbWorker.Join();
		m_loginDbWorker.CloseDB();

		LOG_OK("Shut down of NECROWorld completed.");

		return 0;
	}

	void Server::KeepDatabaseAliveHandler()
	{
		//LOG_DEBUG("KeepDatabaseAliveHandler...");

		// Calls itself again
		m_keepLoginDatabaseAliveTimer.expires_after(std::chrono::milliseconds(m_configSettings.DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS));
		m_keepLoginDatabaseAliveTimer.async_wait([this](boost::system::error_code const& ec) { KeepDatabaseAliveHandler(); });

		// Enqueue a keep alive packet
		DBRequest req(m_ioContext, true);
		req.m_steps.push_back({ static_cast<uint32_t>(LoginDatabaseStatements::KEEP_ALIVE), {} });
		m_loginDbWorker.Enqueue(std::move(req));
	}

}
}
