#ifndef NECRO_NETWORK_THREAD_H
#define NECRO_NETWORK_THREAD_H

#include <thread>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include "FileLogger.h"
#include "ConsoleLogger.h"

#include <iostream>
#include <string>
#include <mutex>

namespace NECRO
{
	inline constexpr uint32_t NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL = 1;

	template<class SocketType>
	class NetworkThread
	{
	private:
		std::shared_ptr<std::mutex>					m_printMutex;
		int											m_threadID;
		std::shared_ptr<std::thread>				m_thread;
		std::atomic<bool>							m_stopped;

		boost::asio::io_context						m_ioContext;
		boost::asio::steady_timer					m_updateTimer;

		std::vector<std::shared_ptr<SocketType>>	m_sockets;
		std::atomic<int32_t>						m_socketsNumber; // tracks both m_sockets and m_queuedSockets size

		std::vector<std::shared_ptr<SocketType>>	m_queuedSockets; // sockets queued up for insertion in the main m_sockets list
		std::mutex									m_queuedSocketsMutex;
	
	public:
		NetworkThread(int id, std::shared_ptr<std::mutex> printMx) : m_printMutex(printMx), m_threadID(id), m_thread(nullptr), m_stopped(false), m_ioContext(1), m_updateTimer(m_ioContext)
		{

		}

	private:
		void Run()
		{
			// Start the thread
			m_updateTimer.expires_after(std::chrono::milliseconds(NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL));
			m_updateTimer.async_wait([this](boost::system::error_code const& ec) { Update(); });

			m_ioContext.run();

			// Here the context went out of work (thread was stopped)
			m_sockets.clear();
			m_queuedSockets.clear();
		}

		void Update()
		{
			if (m_stopped)
				return;

			m_updateTimer.expires_after(std::chrono::milliseconds(NETWORK_THREAD_UPDATE_WAIT_MILLISEC_VAL));
			m_updateTimer.async_wait([this](boost::system::error_code const& ec) { Update(); });

			// Insert pending sockets
			AddQueuedSocketsToMainList();

			// Update the sockets
			// for (auto& sock : m_sockets)
			// sock->Update();

			std::unique_lock lock(*m_printMutex);
			std::cout << m_threadID << " called. " << m_socketsNumber << std::endl;
			lock.unlock();
		}

		void AddQueuedSocketsToMainList()
		{
			std::lock_guard guard(m_queuedSocketsMutex);

			for (std::shared_ptr<SocketType> s : m_queuedSockets)
				m_sockets.push_back(s);

			m_queuedSockets.clear();
		}

	public:
		int Start()
		{
			if (m_thread)
				return -1;

			m_thread = std::make_shared<std::thread>(&NetworkThread::Run, this);

			return 0;
		}

		void GetSocketsSize() const
		{
			return m_socketsNumber;
		}

		void QueueNewSocket(std::shared_ptr<SocketType> newSock)
		{
			std::lock_guard guard(m_queuedSocketsMutex);
			m_queuedSockets.push_back(newSock);
			m_socketsNumber++;
		}

		void Stop()
		{
			m_stopped = true;
			m_ioContext.stop();
		}

		void Join()
		{
			if (m_thread)
				m_thread->join();
		}
	};
}

#endif
