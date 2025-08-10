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
	TCPSocketManager::TCPSocketManager(SocketAddressesFamily _family) : m_listener(_family), m_wakeListen(_family), m_wakeWrite(_family)
	{
		uint16_t inPort = SOCK_MANAGER_SERVER_PORT;
		SocketAddress localAddr(AF_INET, INADDR_ANY, inPort);
		int flag = 1;

		m_listener.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
		m_listener.SetSocketOption(SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));

		m_listener.Bind(localAddr);
		m_listener.SetBlockingEnabled(false);
		m_listener.Listen();


		// This is not strictly needed, but it avoids reallocations for the list vector
		m_list.reserve(NECRO::Auth::MAX_CLIENTS_CONNECTED);

		// Initialize the pollfds vector
		m_listener.m_pfd.fd = m_listener.GetSocketFD();
		m_listener.m_pfd.events = POLLIN;
		m_listener.m_pfd.revents = 0;

		SetupWakeup();
	}

	void TCPSocketManager::SetupWakeup()
	{
		// Make the wake up read listen
		int flag = 1;

		m_wakeListen.Bind(SocketAddress(AF_INET, htonl(INADDR_LOOPBACK), 0));
		m_wakeListen.Listen(1);

		sockaddr_in wakeReadAddr;
		int len = sizeof(wakeReadAddr);
		if (getsockname(m_wakeListen.GetSocketFD(), (sockaddr*)&wakeReadAddr, &len) == SOCKET_ERROR)
		{
			LOG_ERROR("getsockname() failed with error: {}", WSAGetLastError());
			throw std::runtime_error("Failed to get socket address.");
		}

		// Connect on the Write side
		m_wakeWrite.Connect(SocketAddress(*reinterpret_cast<sockaddr*>(&wakeReadAddr)));

		m_wakeWrite.SetBlockingEnabled(false);
		m_wakeWrite.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

		// Accept the connection on the read side
		sock_t accepted = m_wakeListen.AcceptSys();
		m_wakeRead = std::make_unique<TCPSocket>(accepted);

		m_wakeListen.SetBlockingEnabled(false);
		m_wakeListen.SetSocketOption(IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

		m_wakeRead->m_pfd.fd = m_wakeRead->GetSocketFD();
		m_wakeRead->m_pfd.events = POLLIN;
		m_wakeRead->m_pfd.revents = 0;

		m_writePending = false;
	}

	int TCPSocketManager::Poll()
	{
		// Build up the poll_fds vector
		// Remember that these are COPIES of the actual pfd, so changes to members such as m_pollList[i].events will NOT persist
		// m_listener has index 0
		// m_WakeRead has index 1
		// Clients go from SOCK_MANAGER_RESERVED_FDS to n
		std::vector<pollfd> m_pollList;
		m_pollList.push_back(m_listener.m_pfd);
		m_pollList.push_back(m_wakeRead->m_pfd);

		for (auto& s : m_list)
			m_pollList.push_back(s->m_pfd);

		static int timeout = -1;	// wait forever until at least one socket has an event

		LOG_DEBUG("Polling {}", m_list.size());

		int res = WSAPoll(m_pollList.data(), m_pollList.size(), timeout);

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
		if (m_pollList[0].revents != 0)
		{
			if (m_pollList[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("Listener encountered an error.");
				return -1;
			}
			// If there's a reading event for the listener socket, it's a new connection
			else if (m_pollList[0].revents & POLLIN)
			{
				// Check if there's space
				if (m_list.size() < NECRO::Auth::MAX_CLIENTS_CONNECTED)
				{
					SocketAddress otherAddr;
					if (std::shared_ptr<AuthSession> inSock = m_listener.Accept<AuthSession>(otherAddr))
					{
						// IP-based spam prevention
						bool couldBeSpam = false;
						std::string clientIP = inSock->GetRemoteAddress();
						
						auto now = std::chrono::steady_clock::now();

						// Clean old IPs beyond time window
						for(auto it = m_ipRequestMap.begin(); it != m_ipRequestMap.end();)
						{
							if (now - it->second.lastUpdate > std::chrono::minutes(CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN))
								it = m_ipRequestMap.erase(it);
							else
								it++;
						}

						// Check if the requesting IP already made requests in the last time window
						if (m_ipRequestMap.find(clientIP) != m_ipRequestMap.end())
						{
							// If so, update both activity and last try
							m_ipRequestMap[clientIP].lastUpdate = now;
							m_ipRequestMap[clientIP].tries++;

							// If the number of tries exceed the limit, block this request
							if (m_ipRequestMap[clientIP].tries > MAX_CONNECTION_ATTEMPTS_PER_MINUTE)
								couldBeSpam = true;
						}
						else
							m_ipRequestMap.emplace(clientIP, IPRequestData{ now, 1 });

						if (!couldBeSpam)
						{
							LOG_INFO("New connection! Setting up TLS and handshaking...");

							inSock->ServerTLSSetup("localhost");

							// Create a pfd to do handshaking without blocking
							// Initialize status
							inSock->m_status = SocketStatus::HANDSHAKING;
							inSock->SetHandshakeStartTime();
							inSock->UpdateLastActivity();
							m_list.push_back(inSock); // save it in the active list

							// Add the new connection to the pfds
							// TODO, instead of attempting to TLS handshake with POLLOUT events for every socket, let's try to set a global poll timeout and we process TLS handshakes there
							// POLLOUT check for many sockets could be actually slower
							inSock->m_pfd.fd = inSock->GetSocketFD();
							inSock->m_pfd.events = POLLIN | POLLOUT;
							inSock->m_pfd.revents = 0;
							m_pollList.push_back(inSock->m_pfd);
						}
						else
						{
							// Let inSock die
							LOG_INFO("Prevented {} from connecting as it has made too many attempts.", clientIP);
							inSock->Close();
						}
					}
				}
				else
				{
					// if max_client_connected, kick the new client
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

		// Check for WakeUp from DBWorker
		if (m_pollList[1].revents != 0)
		{
			if (m_pollList[1].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_ERROR("WakeUp encountered an error.");
				// TODO handle errors
				// May need to re-do the loopback 
				return -1;
			}
			else if (m_pollList[1].revents & POLLIN)
			{
				// We gotten waken up by the DBWorker

				// Consume what was sent
				static char buf[128];
				m_wakeRead->SysReceive(buf, sizeof(buf));
				// TODO handle errors on receive as well
				// May need to re-do the loopback 

				LOG_DEBUG("Waken up!!! Acquiring mutex and response queue....");
				// Execute the callbacks
				std::queue<DBRequest> requests = g_server.GetDBWorker().GetResponseQueue();
				LOG_DEBUG("Response Queue acquired!");

				while (requests.size() > 0)
				{
					LOG_DEBUG("Executing a callback!");
					DBRequest r = std::move(requests.front());
					requests.pop();
					if (r.m_callback)
						r.m_callback(r.m_sqlRes);
				}

				m_writePending = false;
				LOG_DEBUG("Leaving wake up logic!");
			}
		}

		// Vector of indexes of invalid sockets we'll remove after the client check
		std::vector<int> toRemove;

		// Check for expired sockets
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		// Check for clients
		for (size_t i = SOCK_MANAGER_RESERVED_FDS; i < m_pollList.size(); i++)
		{
			// If the connection hasn't handshaken yet, address a timeout even without any reevent
			if (m_list[i - SOCK_MANAGER_RESERVED_FDS]->m_status == SocketStatus::HANDSHAKING)
			{
				if (now - m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetHandshakeStartTime() > std::chrono::milliseconds(SOCKET_MANAGER_HANDSHAKING_IDLE_TIMEOUT_MS))
				{
					LOG_INFO("TLS connection couldn't handshake for set threshold. Dropping the connection");
					toRemove.push_back(i);
					continue;
				}
			}
			else
			{
				if (now - m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetLastActivity() > std::chrono::milliseconds(SOCKET_MANAGER_POST_TLS_IDLE_TIMEOUT_MS))
				{
					// Timeout the client
					LOG_INFO("Will timeout a client for idling.");
					toRemove.push_back(i);
					continue;
				}
			}

			// Check for actual events
			if (m_pollList[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				LOG_INFO("Client socket error/disconnection detected. Removing it later.");
				toRemove.push_back(i); // i is the fds index, but in connection list it's i-SOCK_MANAGER_RESERVED_FDS
				continue;
			}
			else
			{
				// If the socket has yet to perform the TLS handshake, attempt to progress it
				if (m_list[i - SOCK_MANAGER_RESERVED_FDS]->m_status == SocketStatus::HANDSHAKING)
				{
					if (!(m_pollList[i].revents & (POLLIN | POLLOUT)))
					{
						// no event for this socket, so we skip
						continue;
					}

					// Perform the handshake
					int ret = SSL_accept(m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetSSL());

					if (ret != 1)
					{
						int err = SSL_get_error(m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetSSL(), ret);

						// Keep trying
						if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
							continue;

						if (err == SSL_ERROR_WANT_CONNECT || err == SSL_ERROR_WANT_ACCEPT)
							continue;

						// Errors
						if (err == SSL_ERROR_ZERO_RETURN)
						{
							LOG_INFO("TLS connection closed by peer during handshake.");
							toRemove.push_back(i); // i is the fds index, but in connection list it's i-1
							continue;
						}

						if (err == SSL_ERROR_SYSCALL)
						{
							LOG_ERROR("System call error during TLS handshake. Ret: {}.", ret);
							toRemove.push_back(i);
							continue;
						}

						if (err == SSL_ERROR_SSL)
						{
							if (SSL_get_verify_result(m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetSSL()) != X509_V_OK)
							{
								LOG_ERROR("Verify error: {}\n", X509_verify_cert_error_string(SSL_get_verify_result(m_list[i - SOCK_MANAGER_RESERVED_FDS]->GetSSL())));
								toRemove.push_back(i);
								continue;
							}
						}

						LOG_ERROR("TLSPerformHandshake failed!");
						toRemove.push_back(i);
						continue;
					}
					else
					{
						LOG_OK("TLSPerformHandshake succeeded!");

						// Set status
						m_list[i - SOCK_MANAGER_RESERVED_FDS]->m_status = SocketStatus::GATHER_INFO;
						m_list[i - SOCK_MANAGER_RESERVED_FDS]->UpdateLastActivity();
						m_list[i - SOCK_MANAGER_RESERVED_FDS]->m_pfd.events = POLLIN;
						m_pollList[i].events = POLLIN;
					}
				}
				else // socket successfully performed handshake before
				{
					// If the socket is writable AND we're looking for POLLOUT events as well (meaning there's something on the outQueue), send it!
					if (m_pollList[i].revents & POLLOUT)
					{
						// Check if the POLLOUT is because of the TLS async handshake
						int r = m_list[i - SOCK_MANAGER_RESERVED_FDS]->Send();
						m_list[i - SOCK_MANAGER_RESERVED_FDS]->UpdateLastActivity();

						// If send failed
						if (r < 0)
						{
							LOG_INFO("Client socket error/disconnection detected. Removing it later.");
							toRemove.push_back(i); // i is the fds index, but in connection list it's i-SOCK_MANAGER_RESERVED_FDS
							continue;
						}
					}

					if (m_pollList[i].revents & POLLIN)
					{
						int r = m_list[i - SOCK_MANAGER_RESERVED_FDS]->Receive();
						m_list[i - SOCK_MANAGER_RESERVED_FDS]->UpdateLastActivity();

						// If receive failed, 
						if (r < 0)
						{
							LOG_INFO("Client socket error/disconnection detected. Removing it later.");
							toRemove.push_back(i); // i is the fds index, but in connection list it's i-SOCK_MANAGER_RESERVED_FDS
							continue;
						}
					}
				}
			}
		}

		if (toRemove.size() > 0)
		{
			std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());

			for (int idx : toRemove)
			{
				m_toClose.push_back(m_list[idx - SOCK_MANAGER_RESERVED_FDS]); // m_list contains shared_pointers, so it's ok to not std::move
				m_list.erase(m_list.begin() + (idx - SOCK_MANAGER_RESERVED_FDS));
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
			else // this skips the TLSShutdown (TODO: if connection wasn't meaningufl, we can abrupt disconnect, while if the client was legit, we may wait for the TLS shutdown)
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

	void TCPSocketManager::WakeUp()
	{
		std::lock_guard guard(m_writeMutex);

		if (!m_writePending)
		{
			LOG_DEBUG("TCPSocketManager::WakeUp() called!");
			char dummy = 0;

			// TODO This can be handled better, if SysSend returns EWOULDBLOCK we should attempt to re-send or make the m_wakeWrite block the main thread
			int res = m_wakeWrite.SysSend(&dummy, sizeof(dummy));

			if(res >= 0)
				m_writePending = true;
		}
	}

}
}
