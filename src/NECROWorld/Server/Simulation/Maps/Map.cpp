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

	Entity* Map::AddEntityToMap(std::unique_ptr<Entity> e)
	{
		uint64_t entityGUID = e->GetGUID();

		auto it = m_entities.find(entityGUID);
		if (it == m_entities.end())
		{
			// Calculate entity cell and place it in the cell
			int cellX = e->m_posX / CELL_WIDTH;
			int cellY = e->m_posY / CELL_HEIGHT;

			if (Utility::CellBoundCheck(cellX, cellY, m_width, m_height))
			{
				// Add to cell
				Cell* currentCell = &m_cellMap[cellY * m_width + cellX];
				currentCell->AddEntityHere(e.get()); // note on e.get(): std::move (done later in m_entities.insert) does not change the memory address, so this is safe

				// All worked
				e->SetCurrentMapPtr(this);
				e->SetCurrentCellPtr(currentCell);
				m_entities.insert({ entityGUID, std::move(e) });
				LOG_DEBUG("Entity {} added to map", entityGUID);

				Entity* ePtr = m_entities[entityGUID].get();
				ePtr->OnBeingAddedToMap();
				return ePtr;
			}
			else
			{
				// Invalid cell placement
				LOG_WARNING("Tried to add entity GUID: '{}' to MapID: '{}' - But the position ({}, {}) is out of bounds! Entity is destroyed.", entityGUID, m_mapID, e->m_posX, e->m_posY);
				return nullptr;
			}
		}
		else
		{
			LOG_WARNING("Tried to add entity GUID: '{}' to MapID: '{}' - But that GUID is already present in the m_entities list! Entity is destroyed.", entityGUID, m_mapID);
			return nullptr;
		}
	}

	bool Map::RemoveEntityFromMap(uint64_t entityGUID)
	{
		auto it = m_entities.find(entityGUID);
		if (it == m_entities.end())
		{
			LOG_WARNING("Tried to remove entity GUID: '{}' from MapID: '{}' - But that GUID was not registered here!", entityGUID, m_mapID);
			return false;
		}
		else
		{
			it->second->OnBeingRemovedFromMap();
			it->second->m_currentCell->RemoveEntityHere(entityGUID);
			m_entities.erase(it);
			return true;
		}
	}
}
}
