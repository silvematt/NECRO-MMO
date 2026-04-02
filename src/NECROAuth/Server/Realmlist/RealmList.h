#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mysqlx/xdevapi.h>
#include <shared_mutex>

#include "Realm.h"

namespace NECRO
{
	// --------------------------------------------------------------------------------------------------------------
	// Contains all the information about all the Realms that will be sent to authenticated clients.
	// The list retrieved from the DB and updated every REALMLIST_UPDATE_INTERVAL 
	// --------------------------------------------------------------------------------------------------------------
	class RealmList
	{
	private:
		std::vector<Realm> m_realmlist;
		std::shared_mutex m_mutex;

	public:
		static RealmList& Instance()
		{
			static RealmList rlist;
			return rlist;
		}

		std::vector<Realm> GetRealmList();

		int Init();

		// Called by the main thread during UpdateRealmlistHandler
		bool DBCallback_UpdateRealmList(std::vector<mysqlx::SqlResult>& result);
	};
}
