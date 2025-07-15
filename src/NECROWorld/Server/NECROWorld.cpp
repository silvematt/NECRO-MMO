#include "NECROWorld.h"

#include "SocketUtility.h"
#include "TCPSocket.h"
#include <memory>

namespace NECRO
{
namespace World
{
	Server g_server;

	int Server::Init()
	{
		m_isRunning = false;

		LOG_OK("Booting up NECROServer...");

		SocketUtility::Initialize();

		return 0;
	}

	void Server::Update()
	{
		m_isRunning = true;
		LOG_OK("Server is running...");

		// DEBUG: Just a quick test
		SocketAddress localAddr(AF_INET, INADDR_ANY, 61532);
		TCPSocket listenerSocket(SocketAddressesFamily::INET);

		int flag = 1;
		listenerSocket.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		listenerSocket.Bind(localAddr);

		listenerSocket.Listen();

		std::shared_ptr<TCPSocket> inSock;

		// Engine Loop
		while (m_isRunning)
		{
			// Loop
			SocketAddress otherAddress;
			if (inSock = listenerSocket.Accept(otherAddress))
			{
				LOG_INFO("Somebody just connected!");
			}
		}

		Shutdown();
	}

	void Server::Stop()
	{
		LOG_OK("Stopping the server...");

		m_isRunning = false;
	}

	int Server::Shutdown()
	{
		// Shutdown

		LOG_OK("Shut down of the NECROServer completed.");
		return 0;
	}

}
}
