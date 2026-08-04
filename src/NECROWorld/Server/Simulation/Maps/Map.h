#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "Cell.h"
#include "Entity.h"

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
		// m_mapID is the unique map id that identifies the map/zone (0: virrihael, 1: dungeon, 2: mines, 3: island, etc). 
		// Instanced maps can have the same m_mapID but a different m_instanceID: ([1,123] [1,456] - are the same dungeons but different instances)
		uint32_t m_mapID;

		// Properties filled on Load()
		std::string m_ingameName;
		int m_width;
		int m_height;

		// Map state
		bool m_isActive; // if the map is active or not. Inactive maps will skip updating during a simulation step of

		std::unordered_map<uint64_t, std::unique_ptr<Entity>>	m_entities;
		std::vector<Cell> m_cellMap;

	public:
		Map(uint32_t mapID) : m_isActive(true), m_mapID(mapID)
		{
			// Initialize the m_cellMap in base of the loaded m_mapID
			LoadMap();
		}

		int		LoadMap();
		void	SetActive(bool v);
		void	Update(uint32_t diff);
	};
}
}
