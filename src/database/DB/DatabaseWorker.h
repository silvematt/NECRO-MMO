#ifndef NECRO_DATABASE_WORKER_H
#define NECRO_DATABASE_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "LoginDatabase.h"

namespace NECRO
{
	//-----------------------------------------------------------------------------------------------------
	// An abstraction of a thread that works on a database
	//-----------------------------------------------------------------------------------------------------
	class DatabaseWorker
	{
	private:
		std::unique_ptr<Database>	m_db;
		std::thread					m_thread;

		std::mutex					m_queueMutex;
		std::condition_variable		m_cond;

		std::atomic<bool> m_running{ false };

		// !! Shared Members (access with mutex) !!
		std::queue<mysqlx::SqlStatement> m_q;
		int m_qSize = 0;

	public:
		int Setup(Database::DBType t)
		{
			switch (t)
			{
			case Database::DBType::LOGIN_DATABASE:
				m_db = std::make_unique<LoginDatabase>();
				break;

			default:
				throw std::exception("No database type!");
			}

			if (m_db->Init() == 0)
				return 0;
			else
			{
				LOG_ERROR("Could not initialize DatabaseWorker internal db, MySQL may be not running.");
				return 1;
			}
		}

		//-----------------------------------------------------------------------------------------------------
		// Starts the thread with its ThreadRoutine
		//-----------------------------------------------------------------------------------------------------
		int Start()
		{
			if (m_thread.joinable())
			{
				LOG_WARNING("Attempting to start while thread is still joinable. This should not happen. Trying to stop and join.");
				Stop();
				Join();
			}

			m_running = true;
			m_thread = std::thread(&DatabaseWorker::ThreadRoutine, this);

			return 0;
		}

		//-----------------------------------------------------------------------------------------------------
		// Stops the ThreadRoutine when the queue will empty out
		//-----------------------------------------------------------------------------------------------------
		void Stop()
		{
			{
				std::lock_guard<std::mutex> lock(m_queueMutex);

				m_running = false;
			}
			m_cond.notify_all();
		}

		// ----------------------------------------------------------------------------------------------------
		// Joins the thread when it's possible
		//-----------------------------------------------------------------------------------------------------
		void Join()
		{
			if (m_thread.joinable())
				m_thread.join();
		}

		// ----------------------------------------------------------------------------------------------------
		// Closes the DBConnection session (should happen after worker Joins)
		//-----------------------------------------------------------------------------------------------------
		void CloseDB()
		{
			m_db->Close();
		}

		void Enqueue(mysqlx::SqlStatement&& s)
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_q.push(std::move(s));
			m_qSize++;

			m_cond.notify_all();
		}

		mysqlx::SqlStatement Prepare(int enum_val)
		{
			return m_db->Prepare(enum_val);
		}

		void ThreadRoutine()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(m_queueMutex);

				// If we Stopped the thread and there's nothing in the queue, exit
				if (!m_running && m_qSize <= 0)
				{
					lock.unlock();
					break;
				}

				// Otherwise, we're either still running or there's still something in the queue
				if (m_qSize <= 0) // if we're still running and queue is empty
				{
					// Sleep
					while (m_qSize <= 0 && m_running)
						m_cond.wait(lock);
				}
				else // we have something to run
				{
					// We have lock here

					mysqlx::SqlStatement stmt = std::move(m_q.front());
					m_q.pop();
					m_qSize--;

					lock.unlock();

					// Do stuff
					try
					{
						stmt.execute();
					}
					catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
					{
						std::cerr << "DBWorker MySQL error: " << err.what() << std::endl;
					}
					catch (const std::exception& ex)  // catches standard exceptions
					{
						std::cerr << "DBWorker Standard exception: " << ex.what() << std::endl;
					}
					catch (...)
					{
						std::cerr << "DBWorker Unknown exception caught!" << std::endl;
					}
				}
			}
		}
	};

}
#endif
