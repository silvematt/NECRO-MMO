#ifndef NECRO_LOGIN_DATABASE_H
#define NECRO_LOGIN_DATABASE_H

#include "Database.h"
#include "DBConnection.h"

namespace NECRO
{
	//-------------------------------------------------------
	// Enum of all possible statements
	//-------------------------------------------------------
	enum class LoginDatabaseStatements : uint32_t
	{
		SEL_ACCOUNT_ID_BY_NAME = 0, // name(string)
		CHECK_PASSWORD,				// id(uint32_t)
		INS_LOG_WRONG_PASSWORD,		// id(uint32_t), username (string), ip:port(string)
		DEL_PREV_SESSIONS,			// userid(uint32_t)
		INS_NEW_SESSION,			// userid(uint32_t), sessionKey(binary), authip(string), greetcode(binary)
		UPD_ON_LOGIN,
		KEEP_ALIVE,
		GATHER_REALMS
	};


	//-----------------------------------------------------------------------------------------------------
	// Wrapper for login database connection
	//-----------------------------------------------------------------------------------------------------
	class LoginDatabase : public Database
	{
	public:
		int Init(const std::string& URI) override
		{
			// DBWorker uses pool's client to get the persistent mysql session
			if (m_pool.Init(URI) == 0)
			{
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

		void PrepareAllStatements(mysqlx::Session& s) override
		{
			m_statementsMap.clear();

			PrepareStatement(static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME), s.sql("SELECT id FROM necroauth.users WHERE username = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD), s.sql("SELECT password FROM necroauth.users WHERE id = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD), s.sql("INSERT INTO necroauth.logs_actions (ip, username, action) VALUES (?, ?, ?);"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS), s.sql("DELETE FROM necroauth.active_sessions WHERE userid = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION), s.sql("INSERT INTO necroauth.active_sessions (userid, sessionkey, authip, greetcode) VALUES (?, ?, ?, ?);"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::UPD_ON_LOGIN), s.sql("UPDATE users SET online = ?, last_login = ? WHERE id = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::KEEP_ALIVE), s.sql("SELECT 1"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::GATHER_REALMS), s.sql("SELECT id, name, address, port, status FROM necroauth.realmlist ORDER BY name"));
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
