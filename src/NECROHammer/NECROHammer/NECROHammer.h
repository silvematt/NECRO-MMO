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

	private:
		bool m_isRunning;

		ConsoleLogger	m_cLogger;
		FileLogger		m_fLogger;

		boost::asio::io_context	m_ioContext;

		SocketManager	m_sockManager;

	public:
		ConsoleLogger&		GetConsoleLogger()	{ return m_cLogger; }
		FileLogger&			GetFileLogger()		{ return m_fLogger; }


		int			Init();
		void		Start();
		void		Update();
		void		Stop();
		int			Shutdown();
	};

	// Global access for the Client 
	extern Client g_client;
}
}

#endif