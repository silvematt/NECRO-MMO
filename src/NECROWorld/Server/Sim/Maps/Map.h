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
		bool m_isActive; // if the map is active or not. Inactive maps will skip updating during a simulation step of

		std::vector<std::vector<Cell>> m_cellMap;

	public:
		Map() : m_isActive(true)
		{

		}

		void SetActive(bool v)
		{
			m_isActive = v;

			// Eventual consequences of activating/deactivating a map
		}

		int Init();
		int Start();

		void Update(uint32_t diff);
	};
}
}
