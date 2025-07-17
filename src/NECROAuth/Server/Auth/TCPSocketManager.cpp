#include "TCPSocketManager.h"

#include <NECROServer.h>
#include "AuthSession.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <unordered_map>

namespace NECRO
{
namespace Auth
{
	//-----------------------------------------------------------------------------------------------------
	// Abstracts a TCP Socket Listener into a manager, that listens, accepts and manages connections
	//-----------------------------------------------------------------------------------------------------
	TCPSocketManager::TCPSocketManager(SocketAddressesFamily _family) : m_listener(_family)
	{
		uint16_t inPort = 61531;
		SocketAddress localAddr(AF_INET, INADDR_ANY, inPort);
		int flag = 1;

		m_listener.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		m_listener.SetSocketOption(SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));

		m_listener.Bind(localAddr);
		m_listener.SetBlockingEnabled(false);
		m_listener.Listen();

		// Reserve m_poll_fds space for max amount of clients
		// NEVER GO FURHTER! If an additional element is pushed back, alld the Pdfs pointers in the AuthSession will be broken
		m_poll_fds.reserve(NECRO::Auth::MAX_CLIENTS_CONNECTED);

		// This is not strictly needed, but it avoids reallocations for the list vector
		m_list.reserve(NECRO::Auth::MAX_CLIENTS_CONNECTED);

		// Initialize the pollfds vector
		pollfd pfd;
		pfd.fd = m_listener.GetSocketFD();
		pfd.events = POLLIN;
		pfd.revents = 0;
		m_poll_fds.push_back(pfd);
	}

	int TCPSocketManager::Poll()
	{
		static int timeout = -1;	// wait forever until at least one socket has an event

		LOG_DEBUG("Polling {}", m_list.size());

		int res = WSAPoll(m_poll_fds.data(), m_poll_fds.size(), timeout);

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
		if (m_poll_fds[0].revents != 0)
		{
			if (m_poll_fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("Listener encountered an error.");
				return -1;
			}
			// If there's a reading event for the listener socket, it's a new connection
			else if (m_poll_fds[0].revents & POLLIN)
			{
				// Check if there's space
				if (m_poll_fds.size() < NECRO::Auth::MAX_CLIENTS_CONNECTED)
				{
					SocketAddress otherAddr;
					if (std::shared_ptr<AuthSession> inSock = m_listener.Accept<AuthSession>(otherAddr))
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
								LOG_ERROR("System call error during TLS handshake. Ret: {}.", ret);
								success = false;
								break;
							}

							if (err == SSL_ERROR_SSL)
							{
								if (SSL_get_verify_result(inSock->GetSSL()) != X509_V_OK)
									LOG_ERROR("Verify error: {}\n", X509_verify_cert_error_string(SSL_get_verify_result(inSock->GetSSL())));
							}

							LOG_ERROR("TLSPerformHandshake failed!");
							success = false;
							break;
						}

						if (success)
						{
							LOG_OK("TLSPerformHandshake succeeded!");

							// Initialize status
							inSock->m_status = SocketStatus::GATHER_INFO;
							m_list.push_back(inSock); // save it in the active list

							// Add the new connection to the pfds
							pollfd newPfd;
							newPfd.fd = inSock->GetSocketFD();
							newPfd.events = POLLIN;
							newPfd.revents = 0;
							m_poll_fds.push_back(newPfd);

							// Set inSock PFD pointer with the one we've just created and put in the vector
							inSock->SetPfd(&m_poll_fds[m_poll_fds.size() - 1]);
						}
					}
				} 
				else
				{
					// if max_client_connected, just let the client timeout
					SocketAddress otherAddr;
					if (std::shared_ptr<AuthSession> inSock = m_listener.Accept<AuthSession>(otherAddr))
					{
						LOG_WARNING("Client arrived, but there's no space for him.");
						inSock->Shutdown();
						inSock->Close();
					}
				}
			}
		}

		// Vector of indexes of invalid sockets we'll remove after the client check
		std::vector<int> toRemove;

		// Check for clients
		for (size_t i = 1; i < m_poll_fds.size(); i++)
		{
			if (m_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_INFO("Client socket error/disconnection detected. Removing it later.");
				toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
			}
			else
			{
				// If the socket is writable AND we're looking for POLLOUT events as well (meaning there's something on the outQueue), send it!
				if (m_poll_fds[i].revents & POLLOUT)
				{
					int r = m_list[i - 1]->Send();

					// If send failed
					if (r < 0)
					{
						LOG_INFO("Client socket error/disconnection detected. Removing it later.");
						toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
						continue;
					}
				}

				if (m_poll_fds[i].revents & POLLIN)
				{
					int r = m_list[i - 1]->Receive();

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
				m_toClose.push_back(m_list[idx - 1]); // m_list contains shared_pointers, so it's ok to not std::move

				// Swap the element that we want to delete with the item at the back of the vector
				// Then, we make sure to update the Pfd of the element that was swapped and doesn't have to be deleted
				std::swap(m_poll_fds[idx], m_poll_fds.back());
				std::swap(m_list[idx - 1], m_list.back());

				m_list[idx - 1]->SetPfd(&m_poll_fds[idx]);

				m_poll_fds.pop_back();
				m_list.pop_back();
			}
		}
		toRemove.clear();

		// Cycle check on closing sockets
		for (int i = (int)m_toClose.size() - 1; i >= 0; i--) 
		{
			int rc = m_toClose[i]->TLSShutdown();
			if (rc == 0) 
			{
				// Done shutting down at TLS layer
				m_toClose[i]->Shutdown();  // Shutdown
				m_toClose[i]->Close();     // Close FD & free SSL
				toRemove.push_back(i);

				LOG_OK("TLS shutdown completed.");
			}
			else if (rc < 0) 
			{
				LOG_WARNING("TLSShutdown error idx on socket {}. Shutting it down and removing it...", m_toClose[i]->GetSocketFD());
				m_toClose[i]->Shutdown();  // Shutdown
				m_toClose[i]->Close();     // Close FD & free SSL
				toRemove.push_back(i);
			}
		}

		if (toRemove.size() > 0)
		{
			// Remove sockets marked for deletion (from end to beginning)
			std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());

			for (int idx : toRemove) 
			{
				m_toClose.erase(m_toClose.begin() + idx);
			}
		}
		LOG_DEBUG("Size to close: {}", m_toClose.size());

		return 0;
	}

}
}
