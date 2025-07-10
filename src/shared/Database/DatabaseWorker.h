#ifndef NECRO_DATABASE_WORKER_H
#define NECRO_DATABASE_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include "Database.h"

//-----------------------------------------------------------------------------------------------------
// An abstraction of a thread that works on a database
//-----------------------------------------------------------------------------------------------------
class DatabaseWorker
{
private:
	std::unique_ptr<Database> db;
	std::thread thread;

	std::mutex queueMutex;
	int qSize = 0;
	std::queue<mysqlx::SqlStatement> q;
	std::condition_variable cond;

	std::atomic<bool> running{ false };

public:
	int Setup(Database::DBType t)
	{
		switch (t)
		{
			case Database::DBType::LOGIN_DATABASE:
				db = std::make_unique<LoginDatabase>();
				break;

			default:
				throw std::exception("No database type!");
		}

		if (db->Init() == 0)
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
		if (thread.joinable())
		{
			LOG_WARNING("Attempting to start while thread is still joinable. This should not happen. Trying to stop and join.");
			Stop();
			Join();
		}

		running = true;
		thread = std::thread(&DatabaseWorker::ThreadRoutine, this);

		return 0;
	}

	//-----------------------------------------------------------------------------------------------------
	// Stops the ThreadRoutine when the queue will empty out
	//-----------------------------------------------------------------------------------------------------
	void Stop()
	{
		{
			std::lock_guard<std::mutex> lock(queueMutex);

			running = false;
		}
		cond.notify_all();
	}

	// ----------------------------------------------------------------------------------------------------
	// Joins the thread when it's possible
	//-----------------------------------------------------------------------------------------------------
	void Join()
	{
		if(thread.joinable())
			thread.join();
	}

	// ----------------------------------------------------------------------------------------------------
	// Closes the DBConnection session (should happen after worker Joins)
	//-----------------------------------------------------------------------------------------------------
	void CloseDB()
	{
		db->Close();
	}

	void Enqueue(mysqlx::SqlStatement&& s)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		q.push(std::move(s));
		qSize++;

		cond.notify_all();
	}

	mysqlx::SqlStatement Prepare(int enum_val)
	{
		return db->Prepare(enum_val);
	}

	void ThreadRoutine()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(queueMutex);

			// If we Stopped the thread and there's nothing in the queue, exit
			if (!running && qSize <= 0)
			{
				lock.unlock();
				break;
			}

			// Otherwise, we're either still running or there's still something in the queue
			if (qSize <= 0) // if we're still running and queue is empty
			{
				// Sleep
				while (qSize <= 0 && running)
					cond.wait(lock);
			}
			else // we have something to run
			{
				// We have lock here

				mysqlx::SqlStatement stmt = std::move(q.front());
				q.pop();
				qSize--;

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

#endif
