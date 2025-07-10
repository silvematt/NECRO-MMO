#include "NECROServer.h"

#include "SocketUtility.h"
#include "TCPSocket.h"
#include <memory>

NECROServer server;

int NECROServer::Init()
{
	isRunning = false;

	LOG_OK("Booting up NECROServer...");

	SocketUtility::Initialize();

	return 0;
}

void NECROServer::Update()
{
	isRunning = true;
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
	while (isRunning)
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

void NECROServer::Stop()
{
	LOG_OK("Stopping the server...");

	isRunning = false;
}

int NECROServer::Shutdown()
{
	// Shutdown

	LOG_OK("Shut down of the NECROServer completed.");
	return 0;
}
