#ifndef NECRO_DATABASE_H
#define NECRO_DATABASE_H

#include <unordered_map>

#include "DBConnection.h"
#include "DBConnectionPool.h"

namespace NECRO
{
	//-----------------------------------------------------------------------------------------------------
	// Database basic definition
	//-----------------------------------------------------------------------------------------------------
	class Database
	{
	public:
		enum class DBType
		{
			LOGIN_DATABASE = 0
		};


	protected:
		std::unordered_map<uint32_t, mysqlx::SqlStatement> m_statementsMap;

	public:
		//DBConnection m_conn;
		DBConnectionPool m_pool;


	public:
		virtual int Init(const std::string& URI) = 0;

		//-----------------------------------------------------------------------------------------------------
		// Registers all the statements for the current Database, to be overriden
		//-----------------------------------------------------------------------------------------------------
		virtual void PrepareAllStatements(mysqlx::Session& s) = 0;

		//-----------------------------------------------------------------------------------------------------
		// Registers a Statement [enumval->SQL string]
		//-----------------------------------------------------------------------------------------------------
		void PrepareStatement(uint32_t enumVal, const mysqlx::SqlStatement& statement)
		{
			auto it = m_statementsMap.find(enumVal);
			if (it == m_statementsMap.end())
			{
				m_statementsMap.insert({ enumVal, statement });
			}
		}

		//-----------------------------------------------------------------------------------------------------
		// Returns a mysqlx::SqlStatement, ready to be bound with parameters and executed by the caller
		//-----------------------------------------------------------------------------------------------------
		mysqlx::SqlStatement GetPreparedStatement(uint32_t enum_value)
		{
			auto it = m_statementsMap.find(enum_value);
			if (it == m_statementsMap.end())
				throw std::invalid_argument("Invalid database statement enum: " + std::to_string(enum_value));

			return it->second;
		}

		virtual int Close() = 0;
	};

}

#endif
