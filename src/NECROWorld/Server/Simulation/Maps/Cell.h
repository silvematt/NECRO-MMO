#pragma once

#include <cstdint>
#include <vector>

namespace NECRO
{
namespace World
{
	class Entity;

	//---------------------------------------------------------------------------------
	// A Cell represents a container for each tile in a Map.
	//---------------------------------------------------------------------------------
	class Cell
	{
	private:
		int m_cellX;
		int m_cellY;

		std::vector<Entity*> m_entitiesHere;

	public:

		Cell(int x, int y) : m_cellX(x), m_cellY(y) {}
		void Update(uint32_t diff);

		bool AddEntityHere(Entity* e);
		bool RemoveEntityHere(uint64_t entityGUID);
	};

}
}