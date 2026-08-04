#include "Map.h"

#include "NECROWorld.h"

namespace NECRO
{
namespace World
{
	void Map::SetActive(bool v)
	{
		m_isActive = v;

		// Eventual consequences of activating/deactivating a map
	}

	int Map::LoadMap()
	{
		// Load from DB
		auto mapDb = Server::Instance().GetNDBs().Maps();

		m_ingameName	= *mapDb->TryFind(m_mapID, "MapName")->AsString();
		m_width			= *mapDb->TryFind(m_mapID, "Width")->AsInt();
		m_height		= *mapDb->TryFind(m_mapID, "Height")->AsInt();

		// Apply
		m_cellMap.clear();
		m_cellMap.reserve(m_width*m_height);
		for (int y = 0; y < m_height; y++)
			for (int x = 0; x < m_width; x++)
				m_cellMap.emplace_back(x, y);

		LOG_OK("[MAPS] Loaded: Map ID: '{}' - Name: '{}' loaded! Width:'{}' | Height:'{}'.", m_mapID, m_ingameName, m_width, m_height);
		return 0;
	}

	void Map::Update(uint32_t diff)
	{
		// Let's do a flat update for the whole map for now. The correct way will be to calculate where players are and only update the nearbies
		for (int y = 0; y < m_height; y++)
			for(int x = 0; x < m_width; x++)
			{
				Cell& cell = m_cellMap[y * m_width + x];
				cell.Update(diff);
			}
	}
}
}
