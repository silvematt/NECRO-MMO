#include "WorldSimulation.h"
#include "WorldCmdTypes.h"

#include "CharacterData.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include "GUIDManager.h"
#include "PlayerEntity.h"

namespace NECRO
{
namespace World
{
	// ------------------------------------------------------------------------------
	// These are always assumed to run in the main simulation thread.
	// ------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------------------------------------------------
	// charData is where the player would like to spawn, the actual server-authored 
	// spawn result will be in the returned PlayerSpawnCmdResult.
	// 
	// The server is free to modify charData to ensure correctness, that is what is going to be used for spawning. As soon as this cmd
	// is queued by the WorldSession, ownership of charData is transferred here.
	// --------------------------------------------------------------------------------------------------------------------------------------
	PlayerSpawnCmdResult WorldSimulation::WorldCmd_TryToSpawnPlayerCharacter(CharacterData charData)
	{
		PlayerSpawnCmdResult result;

		// If the zone is in range
		Map* mapToSpawnIn = FindMap(charData.zone);

		// Check the data
		if (mapToSpawnIn)
		{
			// Validate the charData
			int eventualCellX = charData.pos_x / CELL_WIDTH;
			int eventualCellY = charData.pos_y / CELL_HEIGHT;

			// If the position is broken, reposition to the center of the world (this may be the graveyard)
			if (!Utility::CellBoundCheck(eventualCellX, eventualCellY, mapToSpawnIn->GetWidth(), mapToSpawnIn->GetHeight()))
			{
				// Modify data
				charData.pos_x = mapToSpawnIn->GetWidth() / 2 * CELL_WIDTH;
				charData.pos_y = mapToSpawnIn->GetHeight() / 2 * CELL_HEIGHT;
			}

			// Try to spawn the entity
			PlayerEntity* res = static_cast<PlayerEntity*>(mapToSpawnIn->AddEntityToMap(std::move(std::make_unique<PlayerEntity>(GUIDManager::GetNextGUID(), charData))));
			result.playerPtr = res;

			if (res)
			{
				result.success = true;
				result.guid = res->GetGUID();
				result.mapID = mapToSpawnIn->GetMapID();
				result.posX = res->m_posX;
				result.posY = res->m_posY;
				
				if (RegisterPlayer(result.guid, charData.id, res))
				{
					LOG_DEBUG("Spawned player '{}' in MapID: '{}' at position: ({}, {})", charData.characterName, result.mapID, result.posX, result.posY);
					return result;
				}
				// Character spawned but registration failed, undo the spawn!
				else
				{
					LOG_DEBUG("Entity GUID '{}' is getting removed! WorldCmd_TryToSpawnPlayerCharacter failed to register the player!", result.guid);
					mapToSpawnIn->RemoveEntityFromMap(result.guid);
				}
			}
		}
		
		// Every other path that does not hit return result.success = true
		result.success = false;
		return result;
	}

	PlayerDespawnCmdResult WorldSimulation::WorldCmd_TryToDespawnPlayerCharacter(uint64_t guid)
	{
		PlayerDespawnCmdResult result;

		auto it = m_players.find(guid);
		if (it != m_players.end())
		{
			// Despawn the player
			PlayerEntity* p = it->second;
			uint32_t charID = p->GetCharID();

			if (UnregisterPlayer(p->GetGUID(), p->GetCharID()))
			{
				result.success = true;
				LOG_WARNING("Player with GUID: '{}' CharID: '{}' has been unregistered from the WorldSimulation!", guid, charID);

				// Removal from map destroys the player (the maps owns the entities), so we do this at the end
				Map* map = p->GetCurrentMap();
				if (map)
				{
					map->RemoveEntityFromMap(guid);
				}
				else
				{
					// When could this fail? Should never unless we allow p->m_currentMap to be nullptr during map transfers
					LOG_CRITICAL("Despawn Character failed critically!");
				}

				return result;
			}
		}

		// Every other path that does not hit return result.success = true
		result.success = false;
		return result;
	}
}
}
