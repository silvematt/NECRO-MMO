#pragma once

#include "MapDefStore.h"
#include "NDBManager.h"

namespace NECRO
{
	class NDBDataStoreManager
	{
	private:
		MapDefStore m_mapDefStore;

	public:
		int LoadAll(const NDBManager& dbManager)
		{
			auto* mapDB = dbManager.GetDB("maps_db");

			if (mapDB)
			{
				m_mapDefStore.LoadAll(mapDB);
				return 1;
			}

			return 0;
		}

		const MapDefStore& GetMapDefStore() const
		{
			return m_mapDefStore;
		}
	};
}
