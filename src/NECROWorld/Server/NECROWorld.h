#ifndef NECROSERVER_H
#define NECROSERVER_H

#include <boost/asio.hpp>

#include "Config.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "DatabaseWorker.h"

namespace NECRO
{
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

			int NETWORK_THREADS_COUNT = -1; //-1 equals to std::thread::hardware_concurrency()

			std::string LOGIN_DATABASE_URI;
			std::string SESSIONS_DATABASE_URI;
		};

		Server() : m_isRunning(false), m_keepLoginDatabaseAliveTimer(m_ioContext)
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

		boost::asio::io_context m_ioContext;

		DatabaseWorker<LoginDatabase>	m_loginDbWorker;

		// Handlers on main ioContext
		boost::asio::steady_timer m_keepLoginDatabaseAliveTimer;

		void KeepDatabaseAliveHandler();


	public:
		int						Init();
		void					Start();
		void					ApplySettings();
		void					Update();
		void					Stop();
		int						Shutdown();

		DatabaseWorker<LoginDatabase>& GetLoginDBWorker()
		{
			return m_loginDbWorker;
		}

		const ConfigSettings& GetSettings() const
		{
			return m_configSettings;
		}
	};
}
}

#endif
