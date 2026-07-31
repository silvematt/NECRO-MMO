#pragma once

#include <stdint.h>
#include <atomic>
#include <vector>
#include <memory>

#include "Map.h"
#include <chrono>

namespace NECRO
{
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


	public:
		std::atomic<bool> m_isRunning;

		WorldSimulation() : m_isRunning(false), m_worldLoopCounter(0), m_curTime(0), m_prevTime(0), m_curTimeDiff(0)
		{
		}

		int		Start();
		void	Update();
		void	Stop();
	};
}
}
