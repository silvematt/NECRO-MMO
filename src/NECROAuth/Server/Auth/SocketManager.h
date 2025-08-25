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

		// Start triggers the first SocketManagerHandler
		void Start();

		// Handles the Accept loop
		void SocketManagerHandler();

		// Callbacks when accept happen
		void AsyncAcceptCallback(tcp::socket&& sock, int tID);
		void SSLAsyncAcceptCallback(tcp::socket&& sock, int tID);

		// NetworkThreads management
		void StartThreads();
		void StopThreads();
		void JoinThreads();
	};
}
}
#endif
