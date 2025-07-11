#include "TCPSocketManager.h"

#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <unordered_map>

namespace NECRO
{
namespace Auth
{
	std::unordered_map<std::string, AuthSession*> TCPSocketManager::usernameMap;

	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	TCPSocketManager::TCPSocketManager(SocketAddressesFamily _family) : listener(_family)
	{
		uint16_t inPort = 61531;
		SocketAddress localAddr(AF_INET, INADDR_ANY, inPort);
		int flag = 1;

		listener.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		listener.SetSocketOption(SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));

		listener.Bind(localAddr);
		listener.SetBlockingEnabled(false);
		listener.Listen();

		// Initialize the pollfds vector
		pollfd pfd;
		pfd.fd = listener.GetSocketFD();
		pfd.events = POLLIN;
		pfd.revents = 0;
		poll_fds.push_back(pfd);
	}

	int TCPSocketManager::Poll()
	{
		static int timeout = -1;	// wait forever until at least one socket has an event

		LOG_INFO("Polling %d", list.size());
		int res = WSAPoll(poll_fds.data(), poll_fds.size(), timeout);

		// Check for errors
		if (res < 0)
		{
			LOG_ERROR("Could not Poll()");
			return -1;
		}

		// Check for timeout
		if (res == 0)
		{
			return 0;
		}

		// Check for the listener (index 0)
		if (poll_fds[0].revents != 0)
		{
			if (poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("Listener encountered an error.");
				return -1;
			}
			// If there's a reading event for the listener socket, it's a new connection
			else if (poll_fds[0].revents & POLLIN)
			{
				SocketAddress otherAddr;
				if (std::shared_ptr<AuthSession> inSock = listener.Accept<AuthSession>(otherAddr))
				{
					LOG_INFO("New connection! Setting up TLS and handshaking...");

					inSock->ServerTLSSetup("localhost");

					bool success = true;
					int ret = 0;
					// Perform the handshake
					while ((ret = SSL_accept(inSock->GetSSL())) != 1)
					{
						int err = SSL_get_error(inSock->GetSSL(), ret);

						// Keep trying
						if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
							continue;

						if (err == SSL_ERROR_WANT_CONNECT || err == SSL_ERROR_WANT_ACCEPT)
							continue;

						if (err == SSL_ERROR_ZERO_RETURN)
						{
							LOG_INFO("TLS connection closed by peer during handshake.");
							success = false;
							break;
						}

						if (err == SSL_ERROR_SYSCALL)
						{
							LOG_ERROR("System call error during TLS handshake. Ret: %d.", ret);
							success = false;
							break;
						}

						if (err == SSL_ERROR_SSL)
						{
							if (SSL_get_verify_result(inSock->GetSSL()) != X509_V_OK)
								LOG_ERROR("Verify error: %s\n", X509_verify_cert_error_string(SSL_get_verify_result(inSock->GetSSL())));
						}

						LOG_ERROR("TLSPerformHandshake failed!");
						success = false;
						break;
					}

					if (success)
					{
						LOG_OK("TLSPerformHandshake succeeded!");

						// Initialize status
						inSock->status = SocketStatus::GATHER_INFO;
						list.push_back(inSock); // save it in the active list

						// Add the new connection to the pfds
						pollfd newPfd;
						newPfd.fd = inSock->GetSocketFD();
						newPfd.events = POLLIN;
						newPfd.revents = 0;
						poll_fds.push_back(newPfd);

						// Set inSock PFD pointer with the one we've just created and put in the vector
						inSock->SetPfd(&poll_fds[poll_fds.size() - 1]);
					}
				}
			}
		}

		// Vector of indexes of invalid sockets we'll remove after the client check
		std::vector<int> toRemove;

		// Check for clients
		for (size_t i = 1; i < poll_fds.size(); i++)
		{
			if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_INFO("Client socket error/disconnection detected. Removing it later.");
				toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
			}
			else
			{
				// If the socket is writable AND we're looking for POLLOUT events as well (meaning there's something on the outQueue), send it!
				if (poll_fds[i].revents & POLLOUT)
				{
					int r = list[i - 1]->Send();

					// If send failed
					if (r < 0)
					{
						LOG_INFO("Client socket error/disconnection detected. Removing it later.");
						toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
						continue;
					}
				}

				if (poll_fds[i].revents & POLLIN)
				{
					int r = list[i - 1]->Receive();

					// If receive failed, 
					if (r < 0)
					{
						LOG_INFO("Client socket error/disconnection detected. Removing it later.");
						toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
						continue;
					}
				}
			}
		}

		if (toRemove.size() > 0)
		{
			// Remove socket from list and fds, in reverse order to avoid index shift
			std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
			for (int idx : toRemove)
			{
				LOG_INFO("Removing %d", idx);

				list[idx - 1]->Close();
				usernameMap.erase(list[idx - 1]->GetAccountData().username);

				poll_fds.erase(poll_fds.begin() + idx);
				list.erase(list.begin() + (idx - 1));
			}
		}

		return 0;
	}

	bool TCPSocketManager::UsernameIsActive(const std::string& s)
	{
		auto it = usernameMap.find(s);
		if (it == usernameMap.end())
		{
			return false;
		}

		return true;
	}

	void TCPSocketManager::RegisterUsername(const std::string& user, AuthSession* session)
	{
		usernameMap[user] = session;
	}

}
}
