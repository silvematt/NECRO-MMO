#pragma once

#include <cstdint>
#include <string>

#include "NDB.h"

namespace NECRO
{
	// -----------------------------------------------------------------------------------------------------------------------------
	// Description of a map's static content. 
	// 
	// The Layout for Maps is the following:
	// NDB Maps file: contains the absolute data of the map, which defines the fields of the MapDef (id, name, type, etc.) 
	// MapDef.h: holds the absolute data and it's the engine-agnostic description of a map STATIC content
	// Zone.h (client/server): on top of the MapDef, is a loaded map with the cells and the entitis that live in it
	// 
	// Loading order is: NDB->MapDef->Zone
	// -----------------------------------------------------------------------------------------------------------------------------
	class MapDef
	{
	public:
		// m_mapID is the unique map id that identifies the map/zone (0: virrihael, 1: dungeon, 2: mines, 3: island, etc). 
		// Zones are instances of a Map.
		// Instanced maps can have the same m_mapID but be a different Zone loaded in the world simulation
		uint32_t	m_mapID = 0;

		std::string m_mapName;
		int			m_mapType = 0;

		int			m_width = 0;
		int			m_height = 0;
		int			m_nLayers = 0;

		std::string m_mapFileName;

	public:
		MapDef(uint32_t mapID, const NDB* mapDb);
	};
}
