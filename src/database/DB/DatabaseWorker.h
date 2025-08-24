#ifndef NECRO_DATABASE_WORKER_H
#define NECRO_DATABASE_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <vector>

#include "DBRequest.h"
#include "LoginDatabase.h"

namespace NECRO
{
	inline constexpr int DB_REQUEST_TIMEOUT_IF_MYSQL_DOWN_MS = 10000; // This should be the same as the idle-timeout-kick of the server
	//-----------------------------------------------------------------------------------------------------
	// An abstraction of a thread that works on a database
	//-----------------------------------------------------------------------------------------------------
	class DatabaseWorker
	{
	private:
		std::unique_ptr<Database>	m_db;
		std::thread					m_thread;

		std::atomic<bool>			m_running{ false };

		// Producer queue (main thread pushes work here)
		// We use std::vector instead of std::queue because the queues are consumed all at once, so swapping vectors avoids element-by-element removal
		std::vector<DBRequest>		m_externalQueue;
		std::mutex					m_externalQueueMutex;
		std::condition_variable		m_execWakeupCond;

		// Consumer queue
		std::vector<DBRequest>		m_internalQueue;


		std::mutex				m_respMutex;
		std::vector<DBRequest>	m_respQueue;

		std::unique_ptr<mysqlx::Session> m_persistentMysqlSession;

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

			if (CreatePersistentMySQLSession() == 0)
			{
				m_running = true;
				m_thread = std::thread(&DatabaseWorker::ThreadRoutine, this);

				return 0;
			}
			else
				return -1;
		}

		int CreatePersistentMySQLSession()
		{
			// Attempt to open the persistent mysql session
			try
			{
				m_persistentMysqlSession = std::make_unique<mysqlx::Session>(m_db->m_pool.m_client->getSession());
			}
			catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
			{
				std::cerr << "CreatePersistentMySQLSession MySQL error: " << err.what() << std::endl;
				m_persistentMysqlSession.reset();
				return -1;
			}
			catch (const std::exception& ex)  // catches standard exceptions
			{
				std::cerr << "CreatePersistentMySQLSession Standard exception: " << ex.what() << std::endl;
				m_persistentMysqlSession.reset();
				return -1;
			}
			catch (...)
			{
				std::cerr << "CreatePersistentMySQLSession Unknown exception caught!" << std::endl;
				m_persistentMysqlSession.reset();
				return -1;
			}

			return 0;
		}

		int RecreatePersistentMySQLSession()
		{
			// Destroy the mysql session
			if (m_persistentMysqlSession)
			{
				m_persistentMysqlSession->close();
				m_persistentMysqlSession.reset();
			}

			return CreatePersistentMySQLSession();
		}

		//-----------------------------------------------------------------------------------------------------
		// Stops the ThreadRoutine when the queue will empty out
		//-----------------------------------------------------------------------------------------------------
		void Stop()
		{
			// Destroy the mysql session
			if (m_persistentMysqlSession)
			{
				m_persistentMysqlSession->close();
				m_persistentMysqlSession.reset();
			}

			m_running = false;
			m_execWakeupCond.notify_one();
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

		void Enqueue(DBRequest&& req)
		{
			{
				std::lock_guard<std::mutex> lock(m_externalQueueMutex);
				m_externalQueue.push_back(std::move(req));
			}

			m_execWakeupCond.notify_one();
		}

		std::vector<DBRequest> GetResponseQueue()
		{
			std::lock_guard guard(m_respMutex);

			std::vector<DBRequest> toReturn;
			std::swap(toReturn, m_respQueue);
			return toReturn;
		}

		void ThreadRoutine()
		{
			while (true)
			{
				std::unique_lock<std::mutex> lock(m_externalQueueMutex);

				// If we Stopped the thread and there's nothing in the queue, exit
				if (!m_running && m_externalQueue.size() <= 0)
				{
					lock.unlock();
					break;
				}

				// Otherwise, we're either still running or there's still something in the queue
				if (m_externalQueue.size() <= 0) // if we're still running and queue is empty
				{
					// Sleep
					while (m_externalQueue.size() <= 0 && m_running)
						m_execWakeupCond.wait(lock);
				}
				else // we have something to run
				{
					// We have lock here

					// Swap all pending work into internal queue
					std::swap(m_internalQueue, m_externalQueue);
					lock.unlock();

					// Execute the queue
					for (auto& req : m_internalQueue)
					{
						if (!req.m_cancelToken.has_value() || (req.m_cancelToken.has_value() && !req.m_cancelToken->expired()))
						{
							// Do stuff
							try
							{
								// If the persistent session died, attempt to fire it back up as long as the request is servable (idle timeout will not trigger)
								if (!m_persistentMysqlSession)
									while (RecreatePersistentMySQLSession() != 0)
									{
										// If the connection could not be made, wait for one second
										std::this_thread::sleep_for(std::chrono::milliseconds(1000));

										// Then check if it's still worth to try and save this request (idle timeout will not trigger for the socket that initiated this request)
										auto now = std::chrono::steady_clock::now();
										if ((now - req.m_creationTime > std::chrono::milliseconds(DB_REQUEST_TIMEOUT_IF_MYSQL_DOWN_MS)) ||
											(req.m_cancelToken.has_value() && req.m_cancelToken->expired()))
										{
											break; // drop the request
										}

										// Try again
									}

								if (m_persistentMysqlSession)
								{
									// Get a session, prepare the statement and execute it
									mysqlx::SqlStatement stmt = m_db->Prepare(*m_persistentMysqlSession, req.m_enumVal);
									for (auto& param : req.m_bindParams)
										stmt.bind(param);

									req.m_sqlRes = stmt.execute();

									// If the request requires a callback, set things up
									if (!req.m_fireAndForget)
									{
										std::unique_lock<std::mutex> resGuard(m_respMutex);

										// Preserve the life of the m_noticeFunc in this scope before it gets moved
										std::function<void()> func = std::move(req.m_noticeFunc);
										m_respQueue.push_back(std::move(req));

										resGuard.unlock();

										// Call notice function if set
										if (func)
											func();
									}
								}
							}
							catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
							{
								std::cerr << "DBWorker MySQL error: " << err.what() << std::endl;
								RecreatePersistentMySQLSession();
							}
							catch (const std::exception& ex)  // catches standard exceptions
							{
								std::cerr << "DBWorker Standard exception: " << ex.what() << std::endl;
								RecreatePersistentMySQLSession();
							}
							catch (...)
							{
								std::cerr << "DBWorker Unknown exception caught!" << std::endl;
								RecreatePersistentMySQLSession();
							}
						}
					}

					m_internalQueue.clear();
				}
			}
		}
	};

}
#endif
