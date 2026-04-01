#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mysqlx/xdevapi.h>
#include <shared_mutex>

namespace NECRO
{
	// Matches DB table structure necroauth.realmlist
	struct Realm
	{
		uint32_t ID;
		std::string name;
		std::string ip;
		uint16_t port;
		uint8_t status;
	};

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
