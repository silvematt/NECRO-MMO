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
		boost::asio::ssl::context m_sslContext;

	public:
		SocketManager(const uint32_t threadCount, boost::asio::io_context& io) : m_ioContextRef(io), m_sslContext(boost::asio::ssl::context::sslv23_client)
		{
			m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
			m_sslContext.load_verify_file("server.pem");

			m_networkThreadsCount = threadCount;

			// A mutex all threads share to call std::cout
			auto printMutex = std::make_shared<std::mutex>(); 
			for (int i = 0; i < threadCount; i++)
			{
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
			m_distributionTimer = std::make_shared<boost::asio::steady_timer>(m_ioContextRef);

			m_distributionTimer->expires_after(std::chrono::milliseconds(SOCKET_MANAGER_DISTRIBUTION_INTERVAL_MILLISEC));
			m_distributionTimer->async_wait([this](boost::system::error_code const& ec) { DistributeNewSockets(); });
		}

		// ----------------------------------------------------------------------------------------
		// Creates a new socket for each thread and inserts them in their respective queues
		// ----------------------------------------------------------------------------------------
		void DistributeNewSockets()
		{
			m_distributionTimer->expires_after(std::chrono::milliseconds(SOCKET_MANAGER_DISTRIBUTION_INTERVAL_MILLISEC));
			m_distributionTimer->async_wait([this](boost::system::error_code const& ec) { DistributeNewSockets(); });

			for (int i = 0; i < m_networkThreadsCount; i++)
			{
				std::shared_ptr<HammerSocket> newSock = std::make_shared<HammerSocket>(m_ioContextRef, m_sslContext);
				m_networkThreads[i]->QueueNewSocket(newSock);
			}
		}
	};
}
}

#endif
