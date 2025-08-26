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
			std::string clientIP = sock.remote_endpoint().address().to_string();

			auto now = std::chrono::steady_clock::now();

			// Check if the requesting IP already made requests in the last time window
			if (m_ipRequestMap.find(clientIP) != m_ipRequestMap.end())
			{
				// If the number of tries exceed the limit, block this request
				if (m_ipRequestMap[clientIP].tries > config.MAX_CONNECTION_ATTEMPTS_PER_MINUTE)
					couldBeSpam = true;
				else
				{
					// If so, update both activity and last try
					m_ipRequestMap[clientIP].lastUpdate = now;
					m_ipRequestMap[clientIP].tries++;
				}
			}
			else
				m_ipRequestMap.emplace(clientIP, IPRequestData{ now, 1 });

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
				LOG_DEBUG("IP {} made too many requests ({})! Dropping connection.", clientIP, m_ipRequestMap[clientIP].tries);
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

	// Called by boost::asio timer started in Server
	void SocketManager::IPRequestMapCleanup()
	{
		auto now = std::chrono::steady_clock::now();

		// Clean old IPs beyond time window
		for (auto it = m_ipRequestMap.begin(); it != m_ipRequestMap.end();)
		{
			if (now - it->second.lastUpdate > std::chrono::minutes(Server::Instance().GetSettings().CONNECTION_ATTEMPT_CLEANUP_INTERVAL_MIN))
				it = m_ipRequestMap.erase(it);
			else
				it++;
		}
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
