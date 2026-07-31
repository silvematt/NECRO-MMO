#include "SocketManager.h"
#include "NECROWorld.h"

namespace NECRO
{
namespace World
{
void SocketManager::Start()
{
	SocketManagerHandler();
}

// This runs in the AsioThreadPool's io_context
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
	m_acceptor.AsyncAccept<SocketManager, &SocketManager::AsyncAcceptCallback, &SocketManager::OnAcceptError>(this);
}

void SocketManager::AsyncAcceptCallback(tcp::socket&& sock, int tID)
{
	auto config = Server::Instance().GetSettings();
	boost::system::error_code closeEc;

	// Check for max connected clients setting
	if (config.MAX_CONNECTED_CLIENTS_PER_THREAD == -1 || m_networkThreads[tID]->GetSocketsSize() < config.MAX_CONNECTED_CLIENTS_PER_THREAD)
	{		
		// Attempt to get the remote endpoint
		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint endpoint = sock.remote_endpoint(ec);
		if (ec)
		{
			// An error occurred, we bail out 
			sock.close(closeEc);
			SocketManagerHandler();
			return;
		}

		// TODO Does not support IPV6
		uint32_t clientIP = endpoint.address().to_v4().to_uint();
		
		// Check if the requesting IP already made requests in the last time window
		bool couldBeSpam = false;

		if (config.ENABLE_SPAM_PREVENTION)
			couldBeSpam = DoIPSpamPrevention(clientIP);

		if (!couldBeSpam)
		{
			LOG_DEBUG("New client accepted! Put into {}", tID);
			std::shared_ptr<WorldSession> newConn = std::make_shared<WorldSession>(std::move(sock));

			m_networkThreads[tID]->QueueNewSocket(newConn);
		}
		else
		{
			// TODO if iprequestmap size fills, this is spammed as well
			LOG_DEBUG("IP {} made too many requests {}! Dropping connection.", clientIP, config.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL);
			sock.close(closeEc);
		}
	}
	else
	{
		// MAX_CONNECTED_CLIENTS_PER_THREAD reached
		LOG_DEBUG("MAX_CONNECTED_CLIENTS_PER_THREAD reached! Dropping connection.");
		sock.close(closeEc);
	}

	SocketManagerHandler();
}

void SocketManager::OnAcceptError(boost::system::error_code ec, int tID)
{
	LOG_ERROR("Accept failed on tID {}: {}. Calling SocketManagerHandler again.", tID, ec.what());

	// TODO Maybe sleep a bit?

	SocketManagerHandler();
}

// Returns true if the IP is flagged as spam
bool SocketManager::DoIPSpamPrevention(uint32_t clientIP)
{
	auto config = Server::Instance().GetSettings();

	// Acquire mutex on m_ipRequestMap
	{
		std::lock_guard<std::mutex> lock(m_ipRequestMapMutex);

		// Check if the requesting IP already made requests in the last time window (IP_BASED_REQUEST_CLEANUP_INTERVAL_MS)
		auto it = m_ipRequestMap.find(clientIP);
		if (it != m_ipRequestMap.end())
		{
			// If the number of tries exceed the limit, block this request
			if (it->second.tries >= config.MAX_CONNECTION_ATTEMPTS_PER_INTERVAL)
				return true;
			else
			{
				// If so, update both activity and last try
				it->second.tries++;
			}
		}
		else
		{
			if (m_ipRequestMap.size() < IP_REQUEST_MAP_MAX_SIZE)
				m_ipRequestMap.emplace(clientIP, IPRequestData{ 1 });
			else
			{
				// If the map ever reaches IP_REQUEST_MAP_MAX_SIZE with a reasonable window , the server is getting overwhelmed beyond its limits.
				return true;
			}
		}
	} // m_ipRequestMapMutex released

	return false;
}

// This is executed by a ASIOThread, meaning that there could be a datarace with another thread that's doing an AsyncAcceptCallback
void SocketManager::IPRequestMapCleanup()
{
	// Just clear the map after the interval expired
	{
		std::lock_guard<std::mutex> lock(m_ipRequestMapMutex);
		m_ipRequestMap.clear();
	}
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
