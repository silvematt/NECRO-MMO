#ifndef NECROAUTHSERVER_H
#define NECROAUTHSERVER_H

#include <boost/asio.hpp>

#include "Config.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "SocketManager.h"

#include "LoginDatabase.h"
#include "DatabaseWorker.h"

namespace NECRO
{
namespace Auth
{
	inline constexpr uint16_t MAX_CLIENTS_CONNECTED = 5000; //TODO

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
			uint32_t IP_BASED_REQUEST_CLEANUP_INTERVAL_MS = 60000;
			uint32_t DATABASE_CALLBACK_CHECK_INTERVAL_MS = 1000;

			// Server settings
			uint16_t	MANAGER_SERVER_PORT = 61531;
			int			MAX_CONNECTED_CLIENTS_PER_THREAD = -1; //-1 equals to no check
			int			NETWORK_THREADS_COUNT = -1; //-1 equals to std::thread::hardware_concurrency()
			int			CONNECTED_AND_IDLE_TIMEOUT_MS = 10000; // After CONNECTED_AND_IDLE_TIMEOUT_MS, kick the client if he doesn't proceed with the communication
			int			HANDSHAKING_AND_IDLE_TIMEOUT_MS = 10000;

			// Spam prevention
			bool		ENABLE_SPAM_PREVENTION = 1;
			uint32_t	CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = 1;
			uint32_t	MAX_CONNECTION_ATTEMPTS_PER_MINUTE = 10;
		};

		// IP-based spam prevention <ip, last attempt>
		struct IPRequestData//TODO
		{
			std::chrono::steady_clock::time_point lastUpdate;
			size_t tries;
		};

	public:
		Server() :
			m_isRunning(false), m_keepDatabaseAliveTimer(m_ioContext), m_ipRequestCleanupTimer(m_ioContext), m_dbCallbackCheckTimer(m_ioContext)
		{

		}

		static Server& Instance()
		{
			static Server instance;
			return instance;
		}

	private:
		// Status
		bool			m_isRunning;
		ConfigSettings	m_configSettings;

		boost::asio::io_context m_ioContext;
		std::unique_ptr<SocketManager> m_socketManager;

		std::mutex m_ipRequestMapMutex;
		std::unordered_map<std::string, IPRequestData> m_ipRequestMap;

		// directdb will be used for queries that run (and block) on the main thread
		// LoginDatabase	m_directdb;
		DatabaseWorker	m_dbworker;

		// Handlers on main ioContext 
		boost::asio::steady_timer m_keepDatabaseAliveTimer;
		boost::asio::steady_timer m_ipRequestCleanupTimer;
		boost::asio::steady_timer m_dbCallbackCheckTimer;

		void KeepDatabaseAliveHandler();
		void IPRequestCleanupHandler();
		void DBCallbackCheckHandler();

	public:
		//LoginDatabase&	GetDirectDB();
		DatabaseWorker&		GetDBWorker();

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
	};

	inline DatabaseWorker& Server::GetDBWorker()
	{
		return m_dbworker;
	}
}
}

#endif
