#include "WorldSimulation.h"
#include "WorldCmdTypes.h"

#include "CharacterData.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"

#include "GUIDManager.h"
#include "PlayerEntity.h"

#include <memory>

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
	PlayerSpawnCmdResult WorldSimulation::WorldCmd_TryToSpawnPlayerCharacter(CharacterData charData, std::shared_ptr<PlayerPacketQueue> playerPQueue)
	{
		PlayerSpawnCmdResult result;

		// If the zone is in range
		Map* mapToSpawnIn = FindMap(charData.zone);

		// Check the data
		if (mapToSpawnIn)
		{
			// Validate the charData
			int eventualCellX = 0;
			int eventualCellY = 0;
			WorldToCell(charData.pos_x, charData.pos_y, eventualCellX, eventualCellY);

			// If the position is broken, reposition to the center of the world (this may be the graveyard)
			if (!Utility::CellBoundCheck(eventualCellX, eventualCellY, mapToSpawnIn->GetWidth(), mapToSpawnIn->GetHeight()))
			{
				// Modify data
				charData.pos_x = mapToSpawnIn->GetWidth() / 2 * CELL_WIDTH;
				charData.pos_y = mapToSpawnIn->GetHeight() / 2 * CELL_HEIGHT;
				charData.pos_z = 100.01f;
			}

			// Try to spawn the entity
			PlayerEntity* res = static_cast<PlayerEntity*>(mapToSpawnIn->AddEntityToMap(std::move(std::make_unique<PlayerEntity>(GUIDManager::GetNextGUID(), charData, playerPQueue))));

			if (res)
			{
				result.success = true;
				result.guid = res->GetGUID();
				result.mapID = mapToSpawnIn->GetMapID();
				result.posX = res->m_posX;
				result.posY = res->m_posY;
				result.posZ = res->m_posZ;

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
				LOG_WARNING("Player with GUID: '{}' CharID: '{}' has been unregistered from the WorldSimulation!", guid, charID);

				// Removal from map destroys the player (the maps owns the entities), so we do this at the end
				Map* map = p->GetCurrentMap();
				if (map)
				{
					map->RemoveEntityFromMap(guid);
					result.success = true;
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

	// TODO: instead of having to find the player, the worldsession could just pass the pointer and be good given that these are only used in the world thread, but i want to throughly test it
	void WorldSimulation::WorldCmd_TryToUpdatePlayerMovement(uint64_t guid, float_t posX, float_t posY, float_t posZ, uint8_t isoDirection, uint32_t curPacketSeq, uint32_t ackedCorrectionID)
	{
		PlayerEntity* p = FindPlayer(guid);

		if (p)
		{
			if (ackedCorrectionID != p->GetLastCorrectionID())
			{
				LOG_DEBUG("Stale Movement packet! Dropping it silently. acked '{}' - m_lastCorrectionID '{}'", ackedCorrectionID, p->GetLastCorrectionID());
				return;
			}

			// Do all the checks on earth to validate the input
			// Let's refuse /tel from the client or just movements that are too far away from our server-side position
			// TODO: do a speed check, maps bounds, z change validation upon map's definition of points where Z can go up/down, etc.
			if (!std::isfinite(posX) || !std::isfinite(posY) || !std::isfinite(posZ) || // sanitize positions
				isoDirection < 0 || isoDirection >= ISO_DIRECTIONS_N || // check iso direction
				std::abs(posX - p->m_posX) > PLAYER_MOVEMENT_XY_MAX_DIFF_ALLOWED || // check for displacement
				std::abs(posY - p->m_posY) > PLAYER_MOVEMENT_XY_MAX_DIFF_ALLOWED)
			{
				p->SendMovementCorrection(curPacketSeq);
			}
			else
			{
				// Apply
				p->m_posX = posX;
				p->m_posY = posY;
				p->m_posZ = posZ;
				p->m_isoDirection = static_cast<IsoDirection>(isoDirection);
			}
		}
		
		return;
	}
}
}
