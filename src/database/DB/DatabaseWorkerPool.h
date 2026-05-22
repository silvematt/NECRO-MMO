#ifndef DATABASE_WORKER_POOL
#define DATABASE_WORKER_POOL

#include <vector>
#include <string>
#include <memory>

#include "Logger.h"
#include "FileLogger.h"
#include "ConsoleLogger.h"

#include "DatabaseWorker.h"

#include "LoginDatabase.h"
#include "CharactersDatabase.h"

namespace NECRO
{

template<class T>
class DatabaseWorkerPool
{
private:
	std::vector<std::unique_ptr<DatabaseWorker<T>>> m_databases;

public:

	int Setup(int n, const std::string& URI)
	{
		for (int i = 0; i < n; i++)
		{
			m_databases.push_back(std::make_unique<DatabaseWorker<T>>());

			if (m_databases[i]->Setup(URI) != 0)
				return -1;
		}

		return 0;
	}


	int Start()
	{
		for (int i = 0; i < m_databases.size(); i++)
		{
			if (m_databases[i]->Start() != 0)
			{
				LOG_ERROR("Error while starting a DatabaseWorkerPool! Databases could not start.");
				return -1;
			}
		}

		return 0;
	}

	void Stop()
	{
		for (int i = 0; i < m_databases.size(); i++)
			m_databases[i]->Stop();
	}

	void Join()
	{
		for (int i = 0; i < m_databases.size(); i++)
			m_databases[i]->Join();
	}

	void CloseDBs()
	{
		for (int i = 0; i < m_databases.size(); i++)
			m_databases[i]->CloseDB();
	}

	// ------------------------------------------------------------------------------
	// Tries to enqueue the DBRequest in the least-busy DBWorker thread
	// ------------------------------------------------------------------------------
	bool TryEnqueue(DBRequest&& dbRequest)
	{
		int leastBusyIndex = -1;
		size_t curBest = SIZE_MAX;

		for (int i = 0; i < m_databases.size(); i++)
		{
			size_t thisNum = m_databases[i]->GetRequestsSize();
			if (thisNum < curBest)
			{
				curBest = thisNum;
				leastBusyIndex = i;
			}
		}

		return m_databases[leastBusyIndex]->TryEnqueue(std::move(dbRequest));
	}

	// ------------------------------------------------------------------------------
	// Enqueus the DBRequest in the least-busy DBWorker thread
	// ------------------------------------------------------------------------------
	void Enqueue(DBRequest&& dbRequest)
	{
		int leastBusyIndex = -1;
		size_t curBest = SIZE_MAX;

		for (int i = 0; i < m_databases.size(); i++)
		{
			size_t thisNum = m_databases[i]->GetRequestsSize();
			if (thisNum < curBest)
			{
				curBest = thisNum;
				leastBusyIndex = i;
			}
		}

		m_databases[leastBusyIndex]->Enqueue(std::move(dbRequest));
	}

	std::vector<mysqlx::SqlResult> DirectExecute(const DBRequest& req)
	{
		int leastBusyIndex = -1;
		size_t curBest = SIZE_MAX;

		for (int i = 0; i < m_databases.size(); i++)
		{
			size_t thisNum = m_databases[i]->GetRequestsSize();
			if (thisNum < curBest)
			{
				curBest = thisNum;
				leastBusyIndex = i;
			}
		}

		return m_databases[leastBusyIndex]->DirectExecute(std::move(req));
	}


	// ------------------------------------------------------------------------------
	// Enqueus the DBRequest in every 
	// ------------------------------------------------------------------------------
	void EnqueueInAll(DBRequest&& toEnqueue)
	{
		for (int i = 0; i < m_databases.size(); i++)
		{
			DBRequest copy(toEnqueue);
			m_databases[i]->Enqueue(std::move(copy));
		}
	}

	// ------------------------------------------------------------------------------
	// Enqueus the DBRequest in every 
	// ------------------------------------------------------------------------------
	void DirectExecuteInAll(DBRequest&& toEnqueue)
	{
		for (int i = 0; i < m_databases.size(); i++)
		{
			DBRequest copy(toEnqueue);
			m_databases[i]->DirectExecute(std::move(copy));
		}
	}

};
}
#endif
