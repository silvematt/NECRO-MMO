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
		bool isRunning;

		ConsoleLogger cLogger;
		FileLogger fLogger;

	public:
		ConsoleLogger& GetConsoleLogger();
		FileLogger& GetFileLogger();

		int						Init();
		void					Update();
		void					Stop();
		int						Shutdown();
	};

	// Global access for the Server 
	extern Server server;

	// Inline functions
	inline ConsoleLogger& Server::GetConsoleLogger()
	{
		return cLogger;
	}

	inline FileLogger& Server::GetFileLogger()
	{
		return fLogger;
	}

}
}

#endif
