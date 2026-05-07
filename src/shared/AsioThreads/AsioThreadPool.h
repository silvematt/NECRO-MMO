#pragma once

#include <vector>
#include <thread>
#include <memory>

#include "boost/asio.hpp"

namespace NECRO
{
class AsioThreadPool
{
public:
	boost::asio::io_context m_ioContext; // shared with all m_threads

// Members
private:
	std::vector<std::thread> m_threads;
	std::size_t m_poolSize = 1;

	// Prevents .run() from returning when there is no work
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;

private:
	void Join()
	{
		for (auto& thread : m_threads)
		{
			if (thread.joinable())
				thread.join();
		}

		m_threads.clear();
	}

public:
	explicit AsioThreadPool() : m_workGuard(boost::asio::make_work_guard(m_ioContext))
	{
	}

	~AsioThreadPool()
	{
		Stop();
	}

	void Start(std::size_t s)
	{
		if (!m_threads.empty()) 
			return;

		if (s <= 0)
			s = 1;

		m_poolSize = s;

		for (int i = 0; i < m_poolSize; i++)
		{
			m_threads.emplace_back([this](){ m_ioContext.run(); });
		}
	}

	template<typename T>
	void PostWork(T&& work)
	{
		boost::asio::post(m_ioContext, std::forward<T>(work));
	}

	void Stop()
	{
		m_workGuard.reset();
		m_ioContext.stop();
		Join();
	}
};
}
