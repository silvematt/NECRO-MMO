#ifndef NECRO_HAMMER_H
#define NECRO_HAMMER_H

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include "SocketManager.h"

#include <boost/asio.hpp>

namespace NECRO
{
namespace Hammer
{
	inline constexpr const char* CLIENT_CONFIG_FILE_PATH = "hammerclient.conf";

	class Client
	{
	public:
		struct ConfigSettings
		{
			// Threads count to spawn for the SocketManager
			int THREADS_COUNT = -1; //-1 equals to std::thread::hardware_concurrency()
		};

	public:
		Client() : m_isRunning(false)
		{
		}

		static Client& Instance()
		{
			static Client instance;
			return instance;
		}

	private:
		ConfigSettings m_configSettings;
		bool m_isRunning;

		// Main ioContext
		boost::asio::io_context	m_ioContext;

		std::unique_ptr<SocketManager> m_sockManager;

	public:
		// Initializes the Hammer client
		int			Init();

		// Applies the Config settings from the config file loaded during Init
		void		ApplySettings();
		
		void		Start();
		void		Update();
		void		Stop();
		int			Shutdown();
	};
}
}

#endif