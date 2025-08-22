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
		Client(uint32_t threadsCount) : m_isRunning(false), m_sockManager(threadsCount, m_ioContext)
		{
		}

		static Client& Instance()
		{
			static Client instance(std::thread::hardware_concurrency());
			return instance;
		}

	private:
		bool m_isRunning;

		// Main ioContext
		boost::asio::io_context	m_ioContext;

		SocketManager m_sockManager;

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