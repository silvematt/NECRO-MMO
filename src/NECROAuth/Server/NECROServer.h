#ifndef NECROAUTHSERVER_H
#define NECROAUTHSERVER_H

#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "TCPSocketManager.h"

#include "LoginDatabase.h"
#include "DatabaseWorker.h"

namespace NECRO
{
namespace Auth
{
	constexpr uint8_t CLIENT_VERSION_MAJOR = 1;
	constexpr uint8_t CLIENT_VERSION_MINOR = 0;
	constexpr uint8_t CLIENT_VERSION_REVISION = 0;

	class Server
	{
	public:
		Server() :
			m_isRunning(false)
		{

		}

	private:
		// Status
		bool m_isRunning;

		ConsoleLogger	m_cLogger;
		FileLogger		m_fLogger;

		std::unique_ptr<TCPSocketManager> m_sockManager;

		// directdb will be used for queries that run (and block) on the main thread
		// to use only with data-critical code, for example, while making a response packet where an information in the database is needed to go further
		// for "fire-and-forget" operations like updating logs, fields, etc we can use the dbworker
		LoginDatabase	m_directdb;
		DatabaseWorker	m_dbworker;

	public:
		ConsoleLogger&		GetConsoleLogger();
		FileLogger&			GetFileLogger();
		TCPSocketManager&	GetSocketManager();

		LoginDatabase&	GetDirectDB();
		DatabaseWorker&	GetDBWorker();

		int						Init();
		void					Start();
		void					Update();
		void					Stop();
		int						Shutdown();
	};

	// Global access for the Server 
	extern Server g_server;

	// Inline functions
	inline ConsoleLogger& Server::GetConsoleLogger()
	{
		return m_cLogger;
	}

	inline FileLogger& Server::GetFileLogger()
	{
		return m_fLogger;
	}

	inline TCPSocketManager& Server::GetSocketManager()
	{
		return *m_sockManager.get();
	}

	inline LoginDatabase& Server::GetDirectDB()
	{
		return m_directdb;
	}

	inline DatabaseWorker& Server::GetDBWorker()
	{
		return m_dbworker;
	}
}
}

#endif
