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
		UPD_ON_LOGIN
	};


	//-----------------------------------------------------------------------------------------------------
	// Wrapper for login database connection
	//-----------------------------------------------------------------------------------------------------
	class LoginDatabase : public Database
	{
	public:
		int Init() override
		{
			if (m_conn.Init("localhost", 33060, "root", "root") == 0)
				return 0;
			else
				return -1;
		}


		//-----------------------------------------------------------------------------------------------------
		// Returns a mysqlx::SqlStatement, ready to be bound with parameters and executed by the caller
		//-----------------------------------------------------------------------------------------------------
		mysqlx::SqlStatement Prepare(int enum_value) override
		{
			switch (enum_value)
			{
			case static_cast<int>(LoginDatabaseStatements::SEL_ACCOUNT_ID_BY_NAME):
				return m_conn.m_session->sql("SELECT id FROM necroauth.users WHERE username = ?;");
				
			case static_cast<int>(LoginDatabaseStatements::CHECK_PASSWORD):
				return m_conn.m_session->sql("SELECT password FROM necroauth.users WHERE id = ?;"); // TODO password should not be in clear, but should be hashed and salted with the salt saved for each user

			case static_cast<int>(LoginDatabaseStatements::INS_LOG_WRONG_PASSWORD):
				return m_conn.m_session->sql("INSERT INTO necroauth.logs_actions (ip, username, action) VALUES (?, ?, ?);");

			case static_cast<int>(LoginDatabaseStatements::DEL_PREV_SESSIONS):
				return m_conn.m_session->sql("DELETE FROM necroauth.active_sessions WHERE userid = ?;");

			case static_cast<int>(LoginDatabaseStatements::INS_NEW_SESSION):
				return m_conn.m_session->sql("INSERT INTO necroauth.active_sessions (userid, sessionkey, authip, greetcode) VALUES (?, ?, ?, ?);");

			case static_cast<int>(LoginDatabaseStatements::UPD_ON_LOGIN):
				// TODO
				break;

			default:
				throw std::invalid_argument("Invalid LoginDatabaseStatement");
				break;
			}
		}


		//-----------------------------------------------------------------------------------------------------
		// Executes a SqlStatement in a try-catch block and returns a SqlResult
		//-----------------------------------------------------------------------------------------------------
		mysqlx::SqlResult Execute(mysqlx::SqlStatement& statement) override
		{
			try
			{
				return statement.execute();
			}
			catch (const mysqlx::Error& err)  // catches MySQL Connector/C++ specific exceptions
			{
				std::cerr << "MySQL error: " << err.what() << std::endl;
			}
			catch (const std::exception& ex)  // catches standard exceptions
			{
				std::cerr << "Standard exception: " << ex.what() << std::endl;
			}
			catch (...)
			{
				std::cerr << "Unknown exception caught!" << std::endl;
			}
		}

		int Close() override
		{
			m_conn.Close();

			return 0;
		}
	};

}

#endif
