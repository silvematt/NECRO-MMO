#include "Zone.h"

#include "NECROWorld.h"

namespace NECRO
{
namespace World
{
	void Zone::SetActive(bool v)
	{
		m_isActive = v;

		// Eventual consequences of activating/deactivating a Zone
	}

	int Zone::LoadZoneFromMap(uint32_t mapID)
	{
		// Load from NDB
		m_mapDef = Server::Instance().GetNDBStoresManager().GetMapDefStore().GetDef(mapID);

		// Load MapDef
		if (!m_mapDef)
		{
			LOG_ERROR("Could not load MapDef for mapID '{}'", mapID);
			return -1;
		}

		// Apply
		m_cellMap.clear();
		m_cellMap.reserve(m_mapDef->m_width* m_mapDef->m_height);
		uint32_t cellID = 0; // cellIDs are local to the map
		for (int y = 0; y < m_mapDef->m_height; y++)
			for (int x = 0; x < m_mapDef->m_width; x++)
			{
				m_cellMap.emplace_back(cellID, x, y);
				cellID++;
			}

		LOG_INFO("[ZONES] Loaded: ZoneID: '{}' - MapID: '{} | Name: '{}' loaded! Width:'{}' | Height:'{}' - FileName: {}.", m_zoneID, m_mapDef->m_mapID, m_mapDef->m_mapName, m_mapDef->m_width, m_mapDef->m_height, m_mapDef->m_mapFileName);
		return 0;
	}

	void Zone::Update(uint32_t diff)
	{
		// Let's do a flat update for the whole map for now. The correct way will be to calculate where players are and only update the nearbies
		for (int y = 0; y < m_mapDef->m_height; y++)
			for(int x = 0; x < m_mapDef->m_width; x++)
			{
				Cell& cell = m_cellMap[y * m_mapDef->m_width + x];
				cell.Update(diff);
			}

		// After the whole map has been updated (or the TODO POIs, perform the transfer the entities that requested a transfer)
		TransferPendingEntities();
	}

	Entity* Zone::AddEntityToZone(std::unique_ptr<Entity> e)
	{
		uint64_t entityGUID = e->GetGUID();

		auto it = m_entities.find(entityGUID);
		if (it == m_entities.end())
		{
			// Calculate entity cell and place it in the cell
			int cellX = 0;
			int cellY = 0;
			WorldToCell(e->m_posX, e->m_posY, cellX, cellY);

			if (Utility::CellBoundCheck(cellX, cellY, m_mapDef->m_width, m_mapDef->m_height))
			{
				// Add to cell - TODO: add failure for AddEntityHere (maybe because cells have a max amount of entities in it?) and check the return value
				Cell* currentCell = &m_cellMap[cellY * m_mapDef->m_width + cellX];
				currentCell->AddEntityHere(e.get()); // note on e.get(): std::move (done later in m_entities.insert) does not change the memory address, so this is safe

				// All worked
				e->SetCurrentZonePtr(this);
				e->SetCurrentCellPtr(currentCell);
				m_entities.insert({ entityGUID, std::move(e) });
				LOG_DEBUG("Entity {} added to Zone.", entityGUID);

				Entity* ePtr = m_entities[entityGUID].get();
				ePtr->OnBeingAddedToZone();
				return ePtr;
			}
			else
			{
				// Invalid cell placement
				LOG_WARNING("Tried to add entity GUID: '{}' to ZoneID: '{}' - But the position ({}, {}) is out of bounds! Entity is destroyed.", entityGUID, m_zoneID, e->m_posX, e->m_posY);
				return nullptr;
			}
		}
		else
		{
			LOG_WARNING("Tried to add entity GUID: '{}' to ZoneID: '{}' - But that GUID is already present in the m_entities list! Entity is destroyed.", entityGUID, m_zoneID);
			return nullptr;
		}
	}

	bool Zone::RemoveEntityFromZone(uint64_t entityGUID)
	{
		auto it = m_entities.find(entityGUID);
		if (it == m_entities.end())
		{
			LOG_WARNING("Tried to remove entity GUID: '{}' from ZoneID: '{}' - But that GUID was not registered here!", entityGUID, m_zoneID);
			return false;
		}
		else
		{
			it->second->OnBeingRemovedFromZone();
			it->second->m_currentCell->RemoveEntityHere(entityGUID);
			m_entities.erase(it);
			LOG_WARNING("Removed Entity from GUID: '{}' from ZoneID: '{}'!", entityGUID, m_zoneID);
			return true;
		}
	}

	void Zone::AddPendingEntityToTransfer(EntityTransferCtx ctx)
	{
		m_entitiesWaitingForTransfer.push_back(ctx);
	}

	void Zone::TransferPendingEntities()
	{
		for (int i = 0; i < m_entitiesWaitingForTransfer.size(); i++)
		{
			EntityTransferCtx* ctx = &m_entitiesWaitingForTransfer[i];
			Entity* entityToTransfer = FindEntity(ctx->entityToTransferGUID);

			// Check if the entity was destroyed while this was queued (if it died/despawned/teleported etc.)
			if (!entityToTransfer)
				continue;

			if (Utility::CellBoundCheck(ctx->newGridPosX, ctx->newGridPosY, m_mapDef->m_width, m_mapDef->m_height))
			{
				Cell* wantedCell = &m_cellMap[ctx->newGridPosY * m_mapDef->m_width + ctx->newGridPosX];

				if (wantedCell)
				{
					LOG_DEBUG("ZoneID '{}' Transferring Entity GUID: '{}'!", m_zoneID, entityToTransfer->GetGUID());

					// Perform the transfer
					entityToTransfer->m_currentCell->RemoveEntityHere(entityToTransfer->GetGUID());
					entityToTransfer->SetCurrentCellPtr(wantedCell);
					wantedCell->AddEntityHere(entityToTransfer);
					entityToTransfer->OnCellChanges();
				}
				else
				{
					// Failed transfer! If this runs, there's a severe structural issue shouldnt really happen. 
					LOG_ERROR("Critical Error: In ZoneID '{}', checked and sanitized Cell ({},{}) was marked as not in bound in this Zone!", m_zoneID, ctx->newGridPosX, ctx->newGridPosY);
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

	Entity* Zone::FindEntity(uint64_t guid) const
	{
		auto it = m_entities.find(guid);
		if (it == m_entities.end())
			return nullptr;
		else
			return it->second.get();
	}
}
}
