#include "NECROHammer.h"

#include "boost/asio.hpp"

namespace NECRO
{
namespace Hammer
{
	Client g_client(std::thread::hardware_concurrency());

	int Client::Init()
	{
		m_isRunning = false;

		return 0;
	}

	void Client::Start()
	{
		m_isRunning = true;

		m_sockManager.Start();
	}

	void Client::Update()
	{
		// Updating means injecting new sockets in each network thread
		m_sockManager.Update();

		m_ioContext.run();
	}

	void Client::Stop()
	{

	}

	int Client::Shutdown()
	{
		return 0;
	}
}
}