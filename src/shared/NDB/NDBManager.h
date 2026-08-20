#pragma once

#include <unordered_map>
#include <string>
#include "NDB.h"

namespace NECRO
{
	inline const char* NDBS_DEFINITION_FILE_PATH = "NDB/ndbs.def";

	// -------------------------------------------------------------------------------------------------------
	// Holds and manages NDB databases, for both the client and server, dynamically.
	// 
	// NDB Databases will be used by NDBDataStoreManager to load Stores and give us fixed data as a contract 
	// between the game code and the DB fields.
	// 
	// Usage is:
	// NDB->NDBStoreManager->LoadStore()
	// -------------------------------------------------------------------------------------------------------
	class NDBManager
	{
	private:
		std::unordered_map<std::string, NDB> m_dbs;
		bool m_ndbsLoaded = false;

	public:
		int LoadFromDefinition();

		const NDB* GetDB(const std::string& id) const; 

		bool AddDB(const std::string& path);
		bool RemoveDB(const std::string& id);
		void Clear();
	};
}
