#include "WorldSimulation.h"
#include "GUIDManager.h"
#include "Entity.h"
#include "PlayerEntity.h"
#include "NECROWorld.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

#include <boost/asio.hpp>

namespace NECRO
{
namespace World
{
	int WorldSimulation::Start()
	{
		// Load all the default maps
		m_maps.insert({ 0, std::make_unique<Map>(0)}); // 0: Load Virrihael

		// Load the active instanced maps (saved on the DB)

		m_worldLoopCounter = 0;
		m_startTime = std::chrono::steady_clock::now();

		m_isRunning = true;
		return 0;
	}

	void WorldSimulation::Update()
	{
		using namespace std::chrono;

		m_worldLoopCounter++;
		m_curTime = static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - m_startTime).count());
		m_curTimeDiff = m_curTime - m_prevTime;

		// We can throttle here, define a tickrate and have a minDiff before update
		
		// Execute queued cmds
		ExecuteWorldCmds();

		// TODO: This is highly parallelizable, we could have working sim threads working on different maps :D
		for (auto& map : m_maps)
			map.second->Update(m_curTimeDiff);
		
		m_prevTime = m_curTime;
	}

	void WorldSimulation::Stop()
	{
		m_isRunning = false;
	}

	void WorldSimulation::ExecuteWorldCmds()
	{
		std::vector<std::function<void()>> currentQueue;

		{
			std::lock_guard lock(m_cmdsMutex);
			std::swap(m_pendingCmds, currentQueue);
			m_pendingCmds.clear();
		}

		for (auto& cmd : currentQueue)
		{
			try
			{
				cmd();
			}
			catch (...)
			{
				LOG_CRITICAL("Exception caught during WorldCmd execution. Handling: Unknown.");
			}
		}
	}

	void WorldSimulation::PostWorldCmd(std::function<void()> cmd)
	{
		std::lock_guard lock(m_cmdsMutex);
		m_pendingCmds.push_back(std::move(cmd));
	}

	bool WorldSimulation::RegisterPlayer(uint64_t guid, uint32_t charID, PlayerEntity* player)
	{
		auto it = m_charIdToGuid.find(charID);
		if (it == m_charIdToGuid.end())
		{
			// Register the player
			m_players[guid] = player;
			m_charIdToGuid[charID] = guid;
			return true;
		}

		return false;
	}

	bool WorldSimulation::UnregisterPlayer(uint64_t guid, uint32_t charID)
	{
		auto cIt = m_charIdToGuid.find(charID);
		if (cIt == m_charIdToGuid.end() || cIt->second != guid)
			return false;

		m_charIdToGuid.erase(cIt);
		m_players.erase(guid);
		return true;
	}

	Map* WorldSimulation::FindMap(uint32_t mapID)
	{
		auto it = m_maps.find(mapID);
		return it != m_maps.end() ? it->second.get() : nullptr;
	}

	// Note on saving, possible TODO. The current architecture contains a unlikely but possible data race that happens when the player enters the world, leaves it and very quickly reconnects.
	// If the dbworker thread is just 1, it's not a problem because it acts as a FIFO so the UPDATE (save) runs before the SELECT to list the characters.
	// But if the dbworkers are 2, we could have a data race where the select is executed before the enqueued save. I should be extremely rare, but still possible
	bool WorldSimulation::SavePlayerOnDatabase(uint64_t guid)
	{
		PlayerEntity* p = FindPlayer(guid);
		
		if (p)
		{
			const CharacterData* charData = p->GetCharacterData();

			auto& dbWorker = Server::Instance().GetCharactersDBPool();
			{
				DBRequest req(Server::Instance().GetAsioThreadPool().m_ioContext, true);

				req.m_steps.push_back({ static_cast<uint32_t>(CharactersDatabaseStatements::CHAR_SAVE_CHARACTER), { charData->level, charData->xp, charData->zone, charData->pos_x, charData->pos_y, charData->pos_z, charData->id } });
				dbWorker.Enqueue(std::move(req));
			}
		}
		else
			return false;

		return true;
	}

	PlayerEntity* WorldSimulation::FindPlayer(uint64_t guid)
	{
		auto it = m_players.find(guid);
		if (it == m_players.end())
			return nullptr;
		else
			return it->second;
	}
}
}
