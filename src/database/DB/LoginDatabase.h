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
		KEEP_ALIVE
	};


	//-----------------------------------------------------------------------------------------------------
	// Wrapper for login database connection
	//-----------------------------------------------------------------------------------------------------
	class LoginDatabase : public Database
	{
	public:
		int Init(const std::string& URI) override
		{
			if (m_pool.Init(URI) == 0)
			{
				// Register all the statements
				RegisterAllStatements();

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

		void RegisterAllStatements() override
		{
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME), "SELECT id FROM necroauth.users WHERE username = ?;");
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD), "SELECT password FROM necroauth.users WHERE id = ?;");
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD), "INSERT INTO necroauth.logs_actions (ip, username, action) VALUES (?, ?, ?);");
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS), "DELETE FROM necroauth.active_sessions WHERE userid = ?;");
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION), "INSERT INTO necroauth.active_sessions (userid, sessionkey, authip, greetcode) VALUES (?, ?, ?, ?);");
			//RegisterStatement(static_cast<int>(LoginDatabaseStatements::UPD_ON_LOGIN), ""); TODO
			RegisterStatement(static_cast<int>(LoginDatabaseStatements::KEEP_ALIVE), "SELECT 1");
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
