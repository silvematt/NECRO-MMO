#ifndef NECROAUTHSERVER_H
#define NECROAUTHSERVER_H

#include "Config.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "TCPSocketManager.h"

#include "LoginDatabase.h"
#include "DatabaseWorker.h"

namespace NECRO
{
namespace Auth
{
	inline constexpr uint16_t MAX_CLIENTS_CONNECTED = 5000;

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
		};

	public:
		Server() :
			m_isRunning(false)
		{

		}

		static Server& Instance()
		{
			static Server instance;
			return instance;
		}

	private:
		// Status
		ConfigSettings	m_configSettings;
		bool			m_isRunning;

		std::unique_ptr<TCPSocketManager> m_sockManager;

		// directdb will be used for queries that run (and block) on the main thread
		//LoginDatabase	m_directdb;
		DatabaseWorker	m_dbworker;

	public:
		ConsoleLogger&		GetConsoleLogger();
		FileLogger&			GetFileLogger();
		TCPSocketManager&	GetSocketManager();

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
		};
	};

	inline TCPSocketManager& Server::GetSocketManager()
	{
		return *m_sockManager.get();
	}

	/*
	inline LoginDatabase& Server::GetDirectDB()
	{
		return m_directdb;
	}
	*/

	inline DatabaseWorker& Server::GetDBWorker()
	{
		return m_dbworker;
	}
}
}

#endif
