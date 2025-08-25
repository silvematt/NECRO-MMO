#include "SocketManager.h"

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
		LOG_DEBUG("New client accepted! Put into {}", tID);
		std::shared_ptr<AuthSession> newConn = std::make_shared<AuthSession>(std::move(sock), m_networkThreads[tID]->GetSSLContext());

		m_networkThreads[tID]->QueueNewSocket(newConn);

		SocketManagerHandler();
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
