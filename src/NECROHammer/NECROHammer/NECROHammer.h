#ifndef NECRO_HAMMER_H
#define NECRO_HAMMER_H

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
namespace Hammer
{
	class Client
	{
	public:
		Client() : m_isRunning(false)
		{

		}

	private:
		bool m_isRunning;

		ConsoleLogger	m_cLogger;
		FileLogger		m_fLogger;

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