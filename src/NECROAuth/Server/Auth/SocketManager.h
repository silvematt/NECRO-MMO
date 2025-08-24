#ifndef NECRO_SOCKET_MANAGER_H
#define NECRO_SOCKET_MANAGER_H

#include "NetworkThread.h"
#include "AuthSession.h"
#include "TCPAcceptor.h"

#include <boost/asio.hpp>
#include <limits>

namespace NECRO
{
namespace Auth
{
	//-----------------------------------------------------------------------------------------------------
	// Manages the accept logic of new connections and the NetworkThreads
	//-----------------------------------------------------------------------------------------------------
	class SocketManager
	{
		typedef std::vector<std::unique_ptr<NetworkThread<AuthSession>>> NetworkThreadList;

	private:
		boost::asio::io_context&	m_ioContextRef;
		uint32_t					m_threadCount;

		uint32_t			m_networkThreadsCount;
		NetworkThreadList	m_networkThreads;

		// Acceptor
		TCPAcceptor m_acceptor;

	public:
		SocketManager(const uint32_t threadCount, boost::asio::io_context& io, uint16_t port) : m_ioContextRef(io), m_threadCount(threadCount), m_acceptor(m_ioContextRef, port)
		{
			m_networkThreadsCount = threadCount;

			// Spawn the network threads
			for (int i = 0; i < threadCount; i++)
			{
				// Create the threads
				m_networkThreads.push_back(std::make_unique<NetworkThread<AuthSession>>(i, true));
			}

			m_acceptor.Bind();
		}

		void Start()
		{
			SocketManagerHandler();
		}

		void SocketManagerHandler()
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

		void AsyncAcceptCallback(tcp::socket&& sock, int tID)
		{

		}

		
		void SSLAsyncAcceptCallback(tcp::socket&& sock, int tID)
		{
			LOG_DEBUG("New client accepted! Put into {}", tID);
			std::shared_ptr<AuthSession> newConn = std::make_shared<AuthSession>(std::move(sock), m_networkThreads[tID]->GetSSLContext());

			m_networkThreads[tID]->QueueNewSocket(newConn);

			SocketManagerHandler();
		}
		

		void StartThreads()
		{
			for (int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Start();
		}

		void StopThreads()
		{
			for (int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Stop();
		}

		void JoinThreads()
		{
			for (int i = 0; i < m_networkThreadsCount; i++)
				m_networkThreads[i]->Join();
		}
	};
}
}
#endif
