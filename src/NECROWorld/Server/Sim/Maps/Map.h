#pragma once

#include <vector>

#include "Cell.h"

namespace NECRO
{
namespace World
{
	// ------------------------------------------------------------------------------------------------------------------------
	// A loaded map in the Server, can be an exterior, a dungeon or anything in between.
	// 
	// Will always be child of the WorldSimulation. There can be multiple instances of the same map (instanced dungeons)
	// ------------------------------------------------------------------------------------------------------------------------
	class Map
	{
	private:
		// m_mapID is the unique map id that identifies the map/zone (0: exterior world, 1: dungeon, 2: mines, 3: island, etc). 
		// Instanced maps can have the same m_mapID but a different m_instanceID: ([1,123] [1,456] - are the same dungeons but different instances)
		uint32_t m_mapID;

		bool m_isActive; // if the map is active or not. Inactive maps will skip updating during a simulation step of

		std::vector<std::vector<Cell>> m_cellMap;

	public:
		Map() : m_isActive(true)
		{

		}

		Map(uint32_t mapID) : m_isActive(true), m_mapID(mapID)
		{
			// Initialize the m_cellMap in base of the loade m_mapID
		}

		void SetActive(bool v)
		{
			m_isActive = v;

			// Eventual consequences of activating/deactivating a map
		}

		int		Start();
		void	Update(uint32_t diff);
	};
}
}
