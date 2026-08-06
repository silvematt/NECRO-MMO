#pragma once

#include "Database.h"
#include "DBConnection.h"

namespace NECRO
{
	//-------------------------------------------------------
	// Enum of all possible statements
	//-------------------------------------------------------
	enum class CharactersDatabaseStatements : uint32_t
	{
		KEEP_ALIVE = 0,
		CHAR_SEL_ENUM, // accountid(INT) - selects all the characters of the given accountid
		CHAR_INS_CHARACTER,
		CHAR_UPD_CHARACTER,
		CHAR_DEL_CHARACTER,
		CHAR_CHECK_NAME_ALREADY_IN_USE,
		CHAR_CHECK_BELONGS_TO_ACCOUNTID,
		CHAR_DELETE_CHARACTER,
		CHAR_SEL_BY_CHARID
	};

	//-----------------------------------------------------------------------------------------------------
	// Wrapper for login database connection
	//-----------------------------------------------------------------------------------------------------
	class CharactersDatabase : public Database
	{
	public:
		int Init(const std::string& URI) override
		{
			// DBWorker uses pool's client to get the persistent mysql session
			if (m_pool.Init(URI) == 0)
			{
				PrepareAllStatements();
				return 0;
			}
			else
				return -1;
		}

		void PrepareAllStatements() override
		{
			m_statementsMap.clear();

			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::KEEP_ALIVE), ("SELECT 1"));
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_SEL_ENUM), ("SELECT id, name, race, class, gender, level, xp, zone, pos_x, pos_y, pos_z FROM necrochars.characters WHERE accountid = ?;"));
			
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_INS_CHARACTER), "INSERT INTO necrochars.characters (accountid, name, race, class, gender, level, xp, zone, pos_x, pos_y, pos_z) VALUES "
				"(?,?,?,?,?,?,?,?,?,?,?)");

			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_UPD_CHARACTER), "UPDATE necrochars.characters SET name=?,race=?,class=?,gender=?,level=?,xp=?,zone=?,pos_x=?,pos_y=?,pos_z=? " 
				"WHERE id=?");

			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_DEL_CHARACTER), "DELETE FROM necrochars.characters WHERE id = ?");
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_CHECK_NAME_ALREADY_IN_USE), ("SELECT id FROM necrochars.characters WHERE NAME = ?;"));
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_CHECK_BELONGS_TO_ACCOUNTID), ("SELECT name FROM necrochars.characters WHERE id = ? AND accountid = ?;"));
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_DELETE_CHARACTER), "DELETE FROM necrochars.characters WHERE id = ? AND name = ? AND accountid = ?");
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_SEL_BY_CHARID), ("SELECT accountid, name, race, class, gender, level, xp, zone, pos_x, pos_y, pos_z FROM necrochars.characters WHERE id = ?;"));
		}

		int Close() override
		{
			m_pool.Close();
			return 0;
		}
	};
}
