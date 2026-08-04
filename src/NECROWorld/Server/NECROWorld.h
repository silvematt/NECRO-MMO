#pragma once

#include <boost/asio.hpp>

#include "Config.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "DatabaseWorkerPool.h"
#include "AsioThreadPool.h"
#include "SocketManager.h"
#include "WorldSimulation.h"

#include "NDBManager.h"

namespace NECRO
{
// ------------------------------------------------------------------------------------------------------------------------
// NECROWorld:
// 
// - Main Thread runs the WorldLoop, updates the game representation
// - N ASIO threads will handle I/O in a shared io_context, handlers and will distribute connections to NetworkThreads
// - M NetworkThreads will manage the connected clients
// - X DBWorkers threads <Login><Character><World>
// - Optional CLI thread
// ------------------------------------------------------------------------------------------------------------------------
namespace World
{
	inline constexpr const char* WORLD_CONFIG_FILE_PATH = "worldserver.conf";

	class Server
	{
	public:
		// Server settings that can be overridden by config file
		struct ConfigSettings
		{
			uint8_t CLIENT_VERSION_MAJOR = 1;
			uint8_t CLIENT_VERSION_MINOR = 0;
			uint8_t CLIENT_VERSION_REVISION = 0;

			// Handler Updates
			uint32_t DATABASE_ALIVE_HANDLER_UPDATE_INTERVAL_MS = 60000;
			uint32_t IP_BASED_REQUEST_CLEANUP_INTERVAL_MS = 120000;

			// Server Settings
			uint16_t	MANAGER_SERVER_PORT = 61532;
			int			ASIO_THREADS_COUNT = 1;
			int			NETWORK_THREADS_COUNT = 1;
			int			MAX_CONNECTED_CLIENTS_PER_THREAD = -1; //-1 equals to no check
			uint32_t	CONNECTED_AND_IDLE_TIMEOUT_MS = 300000;
			int			LOGIN_DATABASE_THREADS_COUNT = 1;
			int			CHARACTERS_DATABASE_THREADS_COUNT = 1;

			// Spam prevention
			bool		ENABLE_SPAM_PREVENTION = 1;
			uint32_t	CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN = 1;
			uint32_t	MAX_CONNECTION_ATTEMPTS_PER_INTERVAL = 10;

			std::string LOGIN_DATABASE_URI;
			std::string CHARACTERS_DATABASE_URI;
		};

		Server() : m_isRunning(false), m_keepLoginDatabaseAliveTimer(m_asioPool.m_ioContext), m_ipRequestCleanupTimer(m_asioPool.m_ioContext)
		{
		}

		static Server& Instance()
		{
			static Server instance;
			return instance;
		}

	private:
		// Status
		bool m_isRunning;
		ConfigSettings	m_configSettings;

		// Asio - AsioThreadPool owns its own m_ioContext
		AsioThreadPool m_asioPool;
		boost::asio::steady_timer m_keepLoginDatabaseAliveTimer;
		boost::asio::steady_timer m_ipRequestCleanupTimer;

		void KeepDatabasesAliveHandler();
		void IPRequestMapCleanupHandler();

		// NetworkThreads
		std::unique_ptr<SocketManager> m_socketManager;

		// Databases
		DatabaseWorkerPool<LoginDatabase>		m_loginDbPool;
		DatabaseWorkerPool<CharactersDatabase>	m_charactersDBPool;

		// Simulation
		NDBManager		m_ndbs;
		WorldSimulation m_worldSimulation;

	public:
		int						Init();
		void					Start();
		void					ApplySettings();
		void					Update();
		void					Stop();
		int						Shutdown();

		DatabaseWorkerPool<LoginDatabase>& GetLoginDBPool()
		{
			return m_loginDbPool;
		}

		DatabaseWorkerPool<CharactersDatabase>& GetCharactersDBPool()
		{
			return m_charactersDBPool;
		}

		const ConfigSettings& GetSettings() const
		{
			return m_configSettings;
		}
	};
}
}
