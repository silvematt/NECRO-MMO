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
		inet_pton(AF_INET, "192.168.1.221", &addr);

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
		authSocket->m_pfd.fd = authSocket->GetSocketFD();
		authSocket->m_pfd.events = POLLOUT;
		authSocket->m_pfd.revents = 0;

		return 0;
	}

	int AuthManager::NetworkUpdate()
	{
		// If operations never start, no need to poll
		if (!isConnecting && !authSocketConnected)
			return 0;

		std::vector<pollfd> m_pollList;

		m_pollList.push_back(authSocket->m_pfd);

		static int timeout = 0; // waits 0 ms for the poll: try get data now or try next time

		int res = WSAPoll(m_pollList.data(), m_pollList.size(), timeout);

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
			int res = CheckIfAuthConnected(m_pollList[0]); // m_pollList[0] is the authSocket
			if (res == -1)
			{
				LOG_DEBUG("Failed to auth");
				OnDisconnect();
				return res;
			}
		}
		else // Connection has been estabilished
		{
			int res = CheckForIncomingData(m_pollList[0]);
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

		CreateAuthSocket();

		authSocketConnected = false;
		isConnecting = false;
	}

	int AuthManager::CheckIfAuthConnected(pollfd& pfd) // pfd of the authSession
	{
		Console& c = engine.GetConsole();

		// Check for errors
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
			{
				// Get error type and print a message
				if (error == WSAECONNREFUSED)
				{
					LOG_ERROR("Connection refused: server is down or not accepting connections.");
					c.Log("Connection refused: server is down or not accepting connections.");
				}
				else
				{
					LOG_ERROR("AuthSocket encountered an error! {}", error);
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
		if (pfd.revents & POLLOUT)
		{
			// If we get here, it indicates that a non-blocking connect has completed
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
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
				pfd.events = POLLIN;

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

	int AuthManager::CheckForIncomingData(pollfd& pfd)
	{
		Console& c = engine.GetConsole();

		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			int error = 0;
			int len = sizeof(error);
			if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
			{
				LOG_ERROR("AuthSocket encountered an error! {}", error);
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
			if (pfd.revents & POLLOUT)
			{
				int r = authSocket->Send();

				// If receive failed
				if (r < 0)
				{
					int errCode = WSAGetLastError();
					LOG_ERROR("Send: Client socket error/disconnection detected.", errCode);
					return -1;
				}
			}

			if (pfd.revents & POLLIN)
			{
				int r = authSocket->Receive();

				// If receive failed
				if (r < 0)
				{
					int errCode = WSAGetLastError();
					LOG_ERROR("Receive: Client socket error/disconnection detected. {}", errCode);
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
