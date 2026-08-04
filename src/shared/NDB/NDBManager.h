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
	// The "dynamic way" of loading data is suboptimal. TODO: have fixed structs and NDBStores to those 
	// structs that give you instant access to the DB fields. It's also a contract between the game code and 
	// the DB structure.
	// -------------------------------------------------------------------------------------------------------
	class NDBManager
	{
	private:
		std::unordered_map<std::string, NDB> m_dbs;

	public:
		int LoadFromDefinition();

		const NDB* operator[] (const std::string& id) const
		{
			return GetDB(id);
		}

		const NDB* GetDB(const std::string& id) const; 

		bool AddDB(const std::string& path);
		bool RemoveDB(const std::string& id);
	};
}
