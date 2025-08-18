#ifndef NECROHAMMER_SOCKET_MANAGER_H
#define NECROHAMMER_SOCKET_MANAGER_H

#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "NetworkThread.h"
#include "HammerSocket.h"

namespace NECRO
{
namespace Hammer
{
	inline constexpr uint32_t SOCKET_MANAGER_DISTRIBUTION_INTERVAL_MILLISEC = 1;
	inline constexpr uint32_t MAX_SOCKETS_PER_THREAD = 100;

	class SocketManager
	{
	private:
		// Threads
		std::vector<std::shared_ptr<NetworkThread<HammerSocket>>> m_networkThreads;
		int m_networkThreadsCount;

		// Timer to update the distribution of new sockets
		std::shared_ptr<boost::asio::steady_timer> m_distributionTimer;
		
		// Contexts
		boost::asio::io_context& m_ioContextRef;

	public:
		SocketManager(const uint32_t threadCount, boost::asio::io_context& io) : m_ioContextRef(io)
		{
			m_networkThreadsCount = threadCount;

			// Create the distribution timer
			m_distributionTimer = std::make_shared<boost::asio::steady_timer>(m_ioContextRef);

			// A mutex all threads share to call std::cout
			auto printMutex = std::make_shared<std::mutex>(); 
			for (int i = 0; i < threadCount; i++)
			{
				// Create the threads
				m_networkThreads.push_back(std::make_shared<NetworkThread<HammerSocket>>(i, printMutex));
			}
		}

		void Start()
		{
			for(int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Start();
		}

		void Stop()
		{
			for (int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Stop();
		}

		void Join()
		{
			for (int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Join();
		}

		void Update()
		{
			// Start the callback loop
			m_distributionTimer->expires_after(std::chrono::milliseconds(SOCKET_MANAGER_DISTRIBUTION_INTERVAL_MILLISEC));
			m_distributionTimer->async_wait([this](boost::system::error_code const& ec) { DistributeNewSockets(); });
		}

		// ----------------------------------------------------------------------------------------
		// Creates a new socket for each thread and inserts them in their respective queues
		// ----------------------------------------------------------------------------------------
		void DistributeNewSockets()
		{
			// Keep the loop alive
			m_distributionTimer->expires_after(std::chrono::milliseconds(SOCKET_MANAGER_DISTRIBUTION_INTERVAL_MILLISEC));
			m_distributionTimer->async_wait([this](boost::system::error_code const& ec) { DistributeNewSockets(); });

			for (int i = 0; i < m_networkThreadsCount; i++)
			{
				if (m_networkThreads[i]->GetSocketsSize() < MAX_SOCKETS_PER_THREAD)
				{
					// Creates a new socket and sticks it into the network thread's list
					std::shared_ptr<HammerSocket> newSock = std::make_shared<HammerSocket>(m_networkThreads[i]->GetIOContext(), m_networkThreads[i]->GetSSLContext());
					m_networkThreads[i]->QueueNewSocket(newSock);
				}
			}
		}
	};
}
}

#endif
