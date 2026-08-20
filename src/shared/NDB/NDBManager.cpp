#include "NDBManager.h"
#include <fstream>

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
	int NDBManager::LoadFromDefinition()
	{
		LOG_INFO("NDBManager: started loading at '{}'...", NDBS_DEFINITION_FILE_PATH);

		int loadedCount = 0;
		std::ifstream file;
		file.open(NDBS_DEFINITION_FILE_PATH);

		if (file.is_open())
		{
			std::string line;
			while (std::getline(file, line))
			{
				// Skip empty lines
				if (line.empty())
					continue;

				// Delete spaces
				line.erase(remove_if(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }), line.end());

				// Skip comments
				if (line[0] == '#')
					continue;

				if (AddDB("NDB/"+line))
					loadedCount++;
			}
		}
		else
		{
			LOG_ERROR("NDBManager: could not open '{}'", NDBS_DEFINITION_FILE_PATH);
			return 0;
		}

		m_ndbsLoaded = true;

		return loadedCount;
	}

	const NDB* NDBManager::GetDB(const std::string& id) const
	{
		auto it = m_dbs.find(id);
		if (it == m_dbs.end())
		{
			LOG_WARNING("NDBManager: Tried to get NDB with ID: '{}' but it was not loaded!", id);
			return nullptr;
		}
		else
			return &it->second;
	}

	bool NDBManager::AddDB(const std::string& path)
	{
		NDB db;
		if (db.LoadFromDisk(path))
		{
			auto it = m_dbs.find(*db.GetID());
			if (it == m_dbs.end())
			{
				LOG_OK("NDBManager: Loaded '{}'.", *db.GetID());

				m_dbs.insert({ *db.GetID(), std::move(db)});
				return true;
			}
			else
			{
				LOG_WARNING("NDBManager: Could not load '{}'. ID was already in the DB manager!", *db.GetID());
				return false;
			}
		}
		else
		{
			LOG_WARNING("NDBManager: Could not load '{}'.", path);
			return false;
		}
	}

	bool NDBManager::RemoveDB(const std::string& idToRemove)
	{
		int retVal = m_dbs.erase(idToRemove);

		if (retVal == 1)
			return true;

		return false;
	}

	void NDBManager::Clear()
	{
		m_dbs.clear();
		m_ndbsLoaded = false;
	}
}
