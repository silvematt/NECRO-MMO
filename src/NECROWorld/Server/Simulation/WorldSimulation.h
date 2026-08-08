#pragma once

#include <stdint.h>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <mutex>

#include "Map.h"
#include "WorldCmdTypes.h"

namespace NECRO
{
struct CharacterData; // forward declare
namespace World
{
	class PlayerEntity;

	// ------------------------------------------------------------------------
	// Simulation of the whole world. Contains all the maps loaded of the game
	// ------------------------------------------------------------------------
	class WorldSimulation
	{
	private:
		std::atomic<uint32_t> m_worldLoopCounter{0};

		// Tick
		std::chrono::steady_clock::time_point m_startTime;

		uint32_t m_curTime;
		uint32_t m_prevTime;
		uint32_t m_curTimeDiff;

		// All the maps currently loaded loaded in the server
		// Some may be inactive
		std::unordered_map<uint32_t, std::unique_ptr<Map>>	m_maps;

		// All the players in any map. They belong to the map they're currently and are just indexed here for quick access
		// CharID -> GUID
		// GUID -> PlayerEntity*
		std::unordered_map<uint32_t, uint64_t>				m_charIdToGuid;
		std::unordered_map<uint64_t, PlayerEntity*>			m_players;

		// ------------------------------------------------------------------------------------------------------------------------------------------------------
		// A WorldCmd is a function or command that is issued by a WorldSession that requests to execute code on the main thread (simulation thread) and 
		// eventually fire a callback on the thread that originatated the request (that thread may need to know the result of the Cmd to continue its processing)
		// ------------------------------------------------------------------------------------------------------------------------------------------------------
		std::mutex m_cmdsMutex;
		std::vector<std::function<void()>> m_pendingCmds;

	private:
		void	ExecuteWorldCmds();
		bool	RegisterPlayer(uint64_t guid, uint32_t charID, PlayerEntity* player);
		bool	UnregisterPlayer(uint64_t guid, uint32_t charID);
		Map*	FindMap(uint32_t mapID);

	public:
		std::atomic<bool> m_isRunning;

		WorldSimulation() : m_isRunning(false), m_worldLoopCounter(0), m_curTime(0), m_prevTime(0), m_curTimeDiff(0)
		{
		}

		int		Start();
		void	Update();
		void	Stop();

		void	PostWorldCmd(std::function<void()> cmd);

		// Cmds - implemented in Simulation/WorldCmds/x.cpp
		PlayerSpawnCmdResult	WorldCmd_TryToSpawnPlayerCharacter(CharacterData charData);
	};
}
}
