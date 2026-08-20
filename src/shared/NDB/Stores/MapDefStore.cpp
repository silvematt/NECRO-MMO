#include "MapDefStore.h"

namespace NECRO
{
	void MapDefStore::LoadAll(const NDB* mapDb)
	{
		// Load all the maps defined in the map's NDB
		for (auto& it : mapDb->GetRows()) 
		{
			m_defs.insert({ it.first, std::make_unique<MapDef>(it.first, mapDb) });
		}
	}

	const MapDef* MapDefStore::GetDef(uint32_t mapID) const
	{
		auto it = m_defs.find(mapID);
		if (it == m_defs.end())
			return nullptr;
		else
			return it->second.get();
	}
}
