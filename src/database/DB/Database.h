#ifndef NECRO_DATABASE_H
#define NECRO_DATABASE_H

#include "DBConnection.h"

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
		DBConnection m_conn;


	public:
		virtual int Init() = 0;
		virtual mysqlx::SqlStatement Prepare(int enum_val) = 0;
		virtual mysqlx::SqlResult Execute(mysqlx::SqlStatement& statement) = 0;
		virtual int Close() = 0;
	};

}

#endif
