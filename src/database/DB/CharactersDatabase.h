#ifndef NECRO_CHARACTERS_DATABASE_H
#define NECRO_CHARACTERS_DATABASE_H

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

			/*
			if (m_conn.Init("localhost", 33060, "root", "root") == 0)
				return 0;
			else
				return -1;
			*/
		}

		void PrepareAllStatements() override
		{
			m_statementsMap.clear();

			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::KEEP_ALIVE), ("SELECT 1"));
			PrepareStatement(static_cast<int>(CharactersDatabaseStatements::CHAR_SEL_ENUM), ("SELECT id, name, race, class, gender, level, xp, zone, pos_x, pos_y, pos_z FROM necrochars.characters WHERE accountid = ?;"));
		}

		int Close() override
		{
			//m_conn.Close();
			m_pool.Close();

			return 0;
		}
	};

}

#endif
