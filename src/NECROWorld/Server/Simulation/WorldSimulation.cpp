#include "WorldSimulation.h"
#include "GUIDManager.h"
#include "Entity.h"

#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace NECRO
{
namespace World
{
	int WorldSimulation::Start()
	{
		// Load all the default maps
		m_maps.push_back(std::make_unique<Map>(0)); // 0: Load Virrihael

		// Load the active instanced maps (saved on the DB)

		// Entity spawn example
		LOG_DEBUG("Spawning a new PlayerEntity in map {} at ({},{})", 0, 100.f, 170.f);
		m_maps[0]->AddEntityToMap(std::move(std::make_unique<Entity>(GUIDManager::GetNextGUID(), EntityType::PLAYER_ENTITY, 100.f, 170.f)));

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
		m_isRunning = false;
	}
}
}
