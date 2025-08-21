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
	public:
		static Server& Instance()
		{
			static Server instance;
			return instance;
		}

	private:
		// Status
		bool m_isRunning;

	public:
		int						Init();
		void					Update();
		void					Stop();
		int						Shutdown();
	};
}
}

#endif
