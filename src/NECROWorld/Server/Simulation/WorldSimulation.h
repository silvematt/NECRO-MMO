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
		std::vector<std::unique_ptr<Map>> m_maps;

		// ------------------------------------------------------------------------------------------------------------------------------------------------------
		// A WorldCmd is a function or command that is issued by a WorldSession that requests to execute code on the main thread (simulation thread) and 
		// eventually fire a callback on the thread that originatated the request (that thread may need to know the result of the Cmd to continue its processing)
		// ------------------------------------------------------------------------------------------------------------------------------------------------------
		std::mutex m_cmdsMutex;
		std::vector<std::function<void()>> m_pendingCmds;

	private:
		void	ExecuteWorldCmds();

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
		PlayerSpawnCmdResult WorldCmd_TryToSpawnPlayerCharacter(std::shared_ptr<CharacterData> charData);
	};
}
}
