#pragma once

#include <string>
#include <unordered_map>

#include "NDBValue.h"
#include "NDBRow.h"

namespace NECRO
{
	enum class NDBLoadState {NONE, DEFINITION, STRUCTURE, ROWS};

	// -----------------------------------------------------------------------
	// NECRO Database file. 
	// 
	// Definition for database files used by both the client and worldserver.
	// -----------------------------------------------------------------------
	class NDB
	{
	private:
		std::string m_id;

		std::unordered_map<uint32_t, NDBRow>	m_rows;				// ID of the row -> row object. The id of the row is parsed upon loading
		std::unordered_map<std::string, size_t> m_valuesMap;		// column name -> index inside of m_values of the NDBRow

	public:
		NDB()
		{
		}

		// Load a NDB
		bool LoadFromDisk(const std::string & path);

		const NDBValue* TryFind(const NDBRow& row, const std::string& colID) const;
		const NDBValue* TryFind(uint32_t rowID, const std::string& colID) const;
	};
}
