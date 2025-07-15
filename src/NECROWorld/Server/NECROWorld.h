#ifndef NECROSERVER_H
#define NECROSERVER_H

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
namespace World
{
	class Server
	{
	private:
		// Status
		bool m_isRunning;

		ConsoleLogger	m_cLogger;
		FileLogger		m_fLogger;

	public:
		ConsoleLogger&	GetConsoleLogger();
		FileLogger&		GetFileLogger();

		int						Init();
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

}
}

#endif
