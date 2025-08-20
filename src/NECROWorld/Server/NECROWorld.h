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

	public:
		int						Init();
		void					Update();
		void					Stop();
		int						Shutdown();
	};

	// Global access for the Server 
	extern Server g_server;

}
}

#endif
