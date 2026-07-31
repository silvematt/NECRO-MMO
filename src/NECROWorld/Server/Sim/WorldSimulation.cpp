#include "WorldSimulation.h"

namespace NECRO
{
namespace World
{
	int WorldSimulation::Start()
	{
		// Load all the default maps

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

		for (auto& map : m_maps)
			map->Update(m_curTimeDiff);
		
		m_prevTime = m_curTime;
	}

	void WorldSimulation::Stop()
	{

	}
}
}
