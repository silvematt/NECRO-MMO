#pragma once

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
		GATHER_REALMS,
		SEL_SESSIONKEY_BY_GREETCODE,
		INVALIDATE_GREETCODE,
		CREDENTIALS_CHECK			// Credentials check introduced after Proof of Work protocol change, selects the ID and the password
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
				PrepareAllStatements();
				return 0;
			}
			else
				return -1;
		}

		void PrepareAllStatements() override
		{
			m_statementsMap.clear();

			PrepareStatement(static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME), ("SELECT id FROM necroauth.users WHERE username = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD), ("SELECT password FROM necroauth.users WHERE id = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD), ("INSERT INTO necroauth.logs_actions (ip, username, action) VALUES (?, ?, ?);"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS), ("DELETE FROM necroauth.active_sessions WHERE userid = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION), ("INSERT INTO necroauth.active_sessions (userid, sessionkey, authip, greetcode, starttime) VALUES (?, ?, ?, ?, UNIX_TIMESTAMP());")); // TODO, need to properly manage the locale across the whole project
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::UPD_ON_LOGIN), ("UPDATE users SET online = ?, last_login = ? WHERE id = ?;"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::KEEP_ALIVE), ("SELECT 1"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::GATHER_REALMS), ("SELECT id, name, address, port, status FROM necroauth.realmlist ORDER BY name"));
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::SEL_SESSIONKEY_BY_GREETCODE), "SELECT userid, sessionKey, starttime, authip FROM necroauth.active_sessions WHERE greetcode = ?;");
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::INVALIDATE_GREETCODE), "UPDATE necroauth.active_sessions SET greetcode = NULL WHERE greetcode = ?;");
			PrepareStatement(static_cast<int>(LoginDatabaseStatements::CREDENTIALS_CHECK), ("SELECT id, password FROM necroauth.users WHERE username = ?;"));
		}

		int Close() override
		{
			m_pool.Close();
			return 0;
		}
	};
}
