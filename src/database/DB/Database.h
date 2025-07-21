#ifndef NECRO_DATABASE_H
#define NECRO_DATABASE_H

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


	public:
		//DBConnection m_conn;
		DBConnectionPool m_pool;


	public:
		virtual int Init() = 0;

		virtual mysqlx::SqlStatement Prepare(mysqlx::Session& sess, int enum_val) = 0;
		virtual int Close() = 0;
	};

}

#endif
