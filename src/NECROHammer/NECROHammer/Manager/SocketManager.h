#ifndef NECROHAMMER_SOCKET_MANAGER_H
#define NECROHAMMER_SOCKET_MANAGER_H

#include <vector>

#include "NetworkThread.h"
#include "HammerSocket.h"

namespace NECRO
{
namespace Hammer
{
	class SocketManager
	{
	private:
		std::vector<std::shared_ptr<NetworkThread<HammerSocket>>> m_networkThreads;
		
		int m_networkThreadsCount;

	public:
		SocketManager(const uint32_t threadCount)
		{
			m_networkThreadsCount = threadCount;

			auto aaa = std::make_shared<std::mutex>();
			for (int i = 0; i < threadCount; i++)
			{
				m_networkThreads.push_back(std::make_shared<NetworkThread<HammerSocket>>(i, aaa));
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
	};
}
}

#endif
