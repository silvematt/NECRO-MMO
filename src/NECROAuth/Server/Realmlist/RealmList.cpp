#include "RealmList.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <cstdio>

namespace NECRO
{
	std::vector<Realm> RealmList::GetRealmList()
	{
		std::shared_lock lock(m_mutex);
		return m_realmlist; // by copy
	}

	int RealmList::Init()
	{
		m_realmlist.clear();
		return 0;
	}


	bool RealmList::DBCallback_UpdateRealmList(uint32_t ec, std::vector<mysqlx::SqlResult>& result)
	{
		if (ec != 0)
		{
			LOG_ERROR("DBCallback_UpdateRealmList called with error code {}", ec);
			return false;
		}

		LOG_INFO("Executing DBCallback_UpdateRealmList");

		// Build a new list
		std::vector<Realm> newList;

		mysqlx::Row row;
		while ((row = result[0].fetchOne())) //result[0] is the result of step 0 of the dbrequest
		{
			Realm realm;
			realm.ID		= static_cast<uint32_t>(row[0].get<int>());
			realm.name		= row[1].get<std::string>();
			realm.ip		= row[2].get<std::string>();
			realm.port		= static_cast<uint16_t>(row[3].get<int>());
			realm.status	= static_cast<uint8_t>(row[4].get<int>());

			LOG_INFO("Loaded realm: ID={}, Name='{}', Address={}, Port={}, Status={}", realm.ID, realm.name, realm.ip, realm.port, realm.status);

			newList.push_back(std::move(realm));
		}

		LOG_INFO("Loaded {} realm(s) total.", newList.size());

		// Pull new list into the m_realmlist
		{
			std::unique_lock lock(m_mutex);
			m_realmlist.clear();
			m_realmlist = std::move(newList);
		}

		return true;
	}
}
