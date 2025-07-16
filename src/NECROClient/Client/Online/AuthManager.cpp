#include "AuthManager.h"
#include "OpenSSLManager.h"
#include "NECROEngine.h"
#include "AuthCodes.h"

namespace NECRO
{
namespace Client
{
	int AuthManager::Init()
	{
		SocketUtility::Initialize();

		if (OpenSSLManager::ClientInit() != 0)
		{
			LOG_ERROR("Could not initialize OpenSSLManager.");
			return -1;
		}

		CreateAuthSocket();

		return 0;
	}

	void AuthManager::CreateAuthSocket()
	{
		isConnecting = false;

		authSocket = std::make_unique<AuthSession>(SocketAddressesFamily::INET);

		int flag = 1;
		authSocket->SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		authSocket->SetBlockingEnabled(false);
	}

	int AuthManager::ConnectToAuthServer()
	{
		if (isConnecting)
			return 0;

		uint16_t outPort = 61531;
		struct in_addr addr;
		inet_pton(AF_INET, "127.0.0.1", &addr);

		SocketAddress authAddr(AF_INET, addr.s_addr, outPort);

		authSocket->SetRemoteAddressAndPort(authAddr, outPort);

		authSocketConnected = false;
		isConnecting = true;

		if (authSocket->Connect(authAddr) != 0)
		{
			// Manage error
			Console& c = engine.GetConsole();

			LOG_ERROR("Error while attempting to connect.");
			c.Log("Connection failed! Server is down or not accepting connections.");
			OnDisconnect();
			return -1;
		}

		LOG_DEBUG("Attempting to connect to Auth Server...");

		// Initialize the pollfds vector
		pollfd pfd;
		pfd.fd = authSocket->GetSocketFD();
		pfd.events = POLLOUT;
		pfd.revents = 0;
		poll_fds.push_back(pfd);
		authSocket->SetPfd(&poll_fds[poll_fds.size() - 1]);

		return 0;
	}

	int AuthManager::NetworkUpdate()
	{
		// Is there anything to check?
		if (poll_fds.size() <= 0)
			return 0;

		static int timeout = 0; // waits 0 ms for the poll: try get data now or try next time

		int res = WSAPoll(poll_fds.data(), poll_fds.size(), timeout);

		// Check for errors
		if (res < 0)
		{
			LOG_ERROR("Could not Poll()");
			OnDisconnect();
			return -1;
		}

		// Check for timeout
		if (res == 0)
		{
			return 0;
		}

		// Wait for connection
		if (!authSocketConnected)
		{
			int res = CheckIfAuthConnected();
			if (res == -1)
			{
				LOG_DEBUG("Failed to auth");
				OnDisconnect();
				return res;
			}
		}
		else // Connection has been estabilished
		{
			int res = CheckForIncomingData();
			if (res == -1)
			{
				OnDisconnect();
				return res;
			}
		}

		return 0;
	}

	void AuthManager::OnDisconnect()
	{
		// Delete and recreate socket for next try
		authSocket->Close();
		authSocket.reset();
		poll_fds.clear();

		CreateAuthSocket();

		authSocketConnected = false;
		isConnecting = false;
	}

	int AuthManager::CheckIfAuthConnected()
	{
		Console& c = engine.GetConsole();

		// Check for errors
		if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(poll_fds[0].fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
			{
				// Get error type and print a message
				if (error == WSAECONNREFUSED)
				{
					LOG_ERROR("Connection refused: server is down or not accepting connections.");
					c.Log("Connection refused: server is down or not accepting connections.");
				}
				else
				{
					LOG_ERROR("AuthSocket encountered an error!");
					c.Log("Connection lost! Server may have crashed or kicked you.");
				}
			}
			else
			{
				LOG_ERROR("getsockopt failed! [{}]", SocketUtility::GetLastError());
				c.Log("Connection lost! Server may have crashed or kicked you.");
			}

			return -1;
		}

		// Check if the socket is ready for writing, means we either connected successfuly or failed to connect
		if (poll_fds[0].revents & POLLOUT)
		{
			// If we get here, it indicates that a non-blocking connect has completed
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(poll_fds[0].fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
			{
				LOG_ERROR("getsockopt failed! [{}]", SocketUtility::GetLastError());
				return -1;
			}
			else if (error != 0)
			{
				LOG_ERROR("Socket error after connect! [{}]", error);

				// Handle connection error
				if (error == WSAECONNREFUSED)
					LOG_ERROR("Server refused the connection!");

				engine.GetConsole().Log("Server refused the connection!");

				return -1;
			}
			else
			{
				LOG_OK("Connected to the server!");
				engine.GetConsole().Log("Connected to the server!");

				// Switch from POLLOUT to POLLIN to get incoming data, POLLOUT will also be checked but only if there are packets to send in the outQueue
				poll_fds[0].events = POLLIN;

				return OnConnectedToAuthServer();
			}
		}

		return 0;
	}

	int AuthManager::OnConnectedToAuthServer()
	{
		isConnecting = false;
		authSocketConnected = true;

		// Set TLS for the AuthSocket
		authSocket->ClientTLSSetup("localhost");

		engine.GetConsole().Log("Handshaking...");
		if (!engine.GetConsole().IsOpen())
		{
			engine.GetConsole().Toggle();
		}

		// Check if it fails
		if (authSocket->TLSPerformHandshake() == 0)
		{
			LOG_ERROR("Handshake failed!");
			engine.GetConsole().Log("Handshake failed!");
			return -1;
		}

		LOG_OK("TLS Handshake successful!");

		authSocket->OnConnectedCallback(); // here the client will send the greet packet

		return 0;
	}

	int AuthManager::CheckForIncomingData()
	{
		Console& c = engine.GetConsole();

		if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(poll_fds[0].fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
			{
				LOG_ERROR("AuthSocket encountered an error!");
				c.Log("Connection lost! Server may have crashed or kicked you.");
			}
			else
			{
				LOG_ERROR("getsockopt failed! [{}]", SocketUtility::GetLastError());
				c.Log("Connection lost! Server may have crashed or kicked you.");
			}

			return -1;
		}

		else
		{
			// If the socket is writable AND we're looking for POLLOUT events as well (meaning there's something on the outQueue), send it!
			if (poll_fds[0].revents & POLLOUT)
			{
				int r = authSocket->Send();

				// If receive failed
				if (r < 0)
				{
					LOG_ERROR("Client socket error/disconnection detected.");
					return -1;
				}
			}

			if (poll_fds[0].revents & POLLIN)
			{
				int r = authSocket->Receive();

				// If receive failed
				if (r < 0)
				{
					LOG_ERROR("Client socket error/disconnection detected.");
					return -1;
				}
			}
		}

		return 0;
	}

	void AuthManager::OnAuthenticationCompleted()
	{
		data.hasAuthenticated = true;

		// Connect to game server

	}

}
}
