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
		uint32_t cellID = 0; // cellIDs are local to the map
		for (int y = 0; y < m_height; y++)
			for (int x = 0; x < m_width; x++)
			{
				m_cellMap.emplace_back(cellID, x, y);
				cellID++;
			}

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

		// After the whole map has been updated (or the TODO POIs, perform the transfer the entities that requested a transfer)
		TransferPendingEntities();
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
			LOG_WARNING("Removed Entity from GUID: '{}' from MapID: '{}'!", entityGUID, m_mapID);
			return true;
		}
	}

	void Map::AddPendingEntityToTransfer(EntityTransferCtx ctx)
	{
		m_entitiesWaitingForTransfer.push_back(ctx);
	}

	void Map::TransferPendingEntities()
	{
		for (int i = 0; i < m_entitiesWaitingForTransfer.size(); i++)
		{
			EntityTransferCtx* ctx = &m_entitiesWaitingForTransfer[i];
			Entity* entityToTransfer = FindEntity(ctx->entityToTransferGUID);

			// Check if the entity was destroyed while this was queued (if it died/despawned/teleported etc.)
			if (!entityToTransfer)
				continue;

			if (Utility::CellBoundCheck(ctx->newGridPosX, ctx->newGridPosY, m_width, m_height))
			{
				Cell* wantedCell = &m_cellMap[ctx->newGridPosY * m_width + ctx->newGridPosX];

				if (wantedCell)
				{
					LOG_DEBUG("Map '{}' Transferring Entity GUID: '{}'!", m_mapID, entityToTransfer->GetGUID());

					// Perform the transfer
					entityToTransfer->m_currentCell->RemoveEntityHere(entityToTransfer->GetGUID());
					entityToTransfer->SetCurrentCellPtr(wantedCell);
					wantedCell->AddEntityHere(entityToTransfer);
					entityToTransfer->OnCellChanges();
				}
				else
				{
					// Failed transfer! If this runs, there's a severe structural issue shouldnt really happen. 
					LOG_ERROR("Critical Error: In MapID '{}', checked and sanitized Cell ({},{}) was marked as not in bound in this map!", m_mapID, ctx->newGridPosX, ctx->newGridPosY);
				}
			}
			else
			{
				// Failed transfer! This could happen, so we snap the player right back where he was but in the middle of the cell
				entityToTransfer->m_posX = (entityToTransfer->m_curGridPosX * CELL_WIDTH) + HALF_CELL_WIDTH;
				entityToTransfer->m_posY = (entityToTransfer->m_curGridPosY * CELL_HEIGHT) + HALF_CELL_HEIGHT;
				entityToTransfer->OnCellTransferFails(); // Notify entities that they got snapped back and their transfer failed (if it's a PlayerEntity, he probably wants to let the client know)
			}
		}

		m_entitiesWaitingForTransfer.clear();
	}

	Entity* Map::FindEntity(uint64_t guid) const
	{
		auto it = m_entities.find(guid);
		if (it == m_entities.end())
			return nullptr;
		else
			return it->second.get();
	}
}
}
