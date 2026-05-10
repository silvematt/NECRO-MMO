#include "SocketManager.h"
#include "NECROServer.h"

namespace NECRO
{
namespace Auth
{
	void SocketManager::Start()
	{
		SocketManagerHandler();
	}

	// This runs in the main thread's io_context
	void SocketManager::SocketManagerHandler()
	{
		// Selects the socket that will receive
		int tID = -1;
		int minSockNum = std::numeric_limits<int>::max();

		for (int i = 0; i < m_networkThreadsCount; i++)
			if (minSockNum > m_networkThreads[i]->GetSocketsSize())
			{
				tID = i;
				minSockNum = m_networkThreads[i]->GetSocketsSize();
			}

		m_acceptor.SetInSocket(m_networkThreads[tID]->GetAcceptSocketPtr(), tID);
		m_acceptor.AsyncAccept<SocketManager, &SocketManager::SSLAsyncAcceptCallback>(this);
	}

	void SocketManager::SSLAsyncAcceptCallback(tcp::socket&& sock, int tID)
	{
		auto config = Server::Instance().GetSettings();

		// Check for max connected clients setting
		if (config.MAX_CONNECTED_CLIENTS_PER_THREAD == -1 || m_networkThreads[tID]->GetSocketsSize() < config.MAX_CONNECTED_CLIENTS_PER_THREAD)
		{
			// IP-based spam prevention
			bool couldBeSpam = false;

			// Attempt to get the remote endpoint
			boost::system::error_code ec;
			boost::asio::ip::tcp::endpoint endpoint = sock.remote_endpoint(ec);
			if (ec)
			{
				// An error occurred, bail out
				SocketManagerHandler();
				return;
			}
			std::string clientIP = endpoint.address().to_string();

			auto now = std::chrono::steady_clock::now();

			// Check if the requesting IP already made requests in the last time window (IP_BASED_REQUEST_CLEANUP_INTERVAL_MS)
			auto it = m_ipRequestMap.find(clientIP);
			if (it != m_ipRequestMap.end())
			{
				// If the number of tries exceed the limit, block this request
				if (it->second.tries > config.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL)
					couldBeSpam = true;
				else
				{
					// If so, update both activity and last try
					it->second.lastUpdate = now;
					it->second.tries++;
				}
			}
			else
			{
				if (m_ipRequestMap.size() < IP_REQUEST_MAP_MAX_SIZE)
				{
					m_ipRequestMap.emplace(clientIP, IPRequestData{ now, 1 });
				}
				else
				{
					// TODO this log must happen once per cleanup cycle, otherwise if the map fills and requests keep coming we keep wasting time logging this
					LOG_DEBUG("Reached {} in IPRequestMap", IP_REQUEST_MAP_MAX_SIZE);
					couldBeSpam = true;
				}
			}

			if (!config.ENABLE_SPAM_PREVENTION)
				couldBeSpam = false;

			if (!couldBeSpam)
			{
				LOG_DEBUG("New client accepted! Put into {}", tID);
				std::shared_ptr<AuthSession> newConn = std::make_shared<AuthSession>(std::move(sock), m_networkThreads[tID]->GetSSLContext());

				m_networkThreads[tID]->QueueNewSocket(newConn);
			}
			else
			{
				// TODO if iprequestmap size fills, this is spammed as well
				LOG_DEBUG("IP {} made too many requests {}! Dropping connection.", clientIP, config.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL);
				sock.close();
			}
		}
		else
		{
			// MAX_CONNECTED_CLIENTS_PER_THREAD reached
			LOG_DEBUG("MAX_CONNECTED_CLIENTS_PER_THREAD reached! Dropping connection.");
			sock.close();
		}

		SocketManagerHandler();
	}

	// Called by boost::asio timer started in Server's main thread
	void SocketManager::IPRequestMapCleanup()
	{
		// Just clear the map after the interval
		m_ipRequestMap.clear();
	}

	void SocketManager::AsyncAcceptCallback(tcp::socket&& sock, int tID)
	{

	}

	void SocketManager::StartThreads()
	{
		for (int i = 0; i < m_networkThreadsCount; i++)
			m_networkThreads[i]->Start();
	}

	void SocketManager::StopThreads()
	{
		for (int i = 0; i < m_networkThreadsCount; i++)
			m_networkThreads[i]->Stop();
	}

	void SocketManager::JoinThreads()
	{
		for (int i = 0; i < m_networkThreadsCount; i++)
			m_networkThreads[i]->Join();
	}

	
}
}
