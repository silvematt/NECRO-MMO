#pragma once

#include "NetworkThread.h"
#include "TCPAcceptor.h"

#include "WorldSession.h"

#include <boost/asio.hpp>
#include <limits>
#include <IPRequestData.h>

namespace NECRO
{
namespace World
{
class SocketManager
{
	typedef std::vector<std::unique_ptr<NetworkThread<WorldSession>>> NetworkThreadList;

private:
	boost::asio::io_context& m_ioContextRef; // ASIO's io_context reference, SocketManagerHandler runs there

	uint32_t			m_networkThreadsCount;
	NetworkThreadList	m_networkThreads;

	// Acceptor
	TCPAcceptor m_acceptor;

	// IP-Request spam prevention
	std::mutex	m_ipRequestMapMutex; // In the WorldServer, IPRequestMapCleanup is executed by a ASIOThread, meaning that there could be a datarace with another thread that's doing an AsyncAcceptCallback
									 // In the AuthServer the mutex is not needed because only the main thread runs AsyncAcceptCallback and IPRequestMapCleanup so only one can happen at a given time
	std::unordered_map<std::string, IPRequestData> m_ipRequestMap;

public:
	SocketManager(const uint32_t threadCount, boost::asio::io_context& io, uint16_t port) : m_ioContextRef(io), m_acceptor(m_ioContextRef, port)
	{
		m_networkThreadsCount = threadCount;

		// Create the network threads
		for (int i = 0; i < threadCount; i++)
		{
			m_networkThreads.push_back(std::make_unique<NetworkThread<WorldSession>>(i, true));
		}

		m_acceptor.Bind();
	}

	// Start triggers the first SocketManagerHandler
	void Start();

	// Handles the Accept loop
	void SocketManagerHandler();

	// Callbacks when accept happen
	void AsyncAcceptCallback(tcp::socket&& sock, int tID);

	void IPRequestMapCleanup();

	// NetworkThreads management
	void StartThreads();
	void StopThreads();
	void JoinThreads();
};
}
}
