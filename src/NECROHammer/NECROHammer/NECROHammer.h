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

		boost::asio::io_context	m_ioContext;

		SocketManager m_sockManager;

	public:
		int			Init();
		void		Start();
		void		Update();
		void		Stop();
		int			Shutdown();
	};
}
}

#endif