#pragma once

#include <cstdint>
#include <vector>

namespace NECRO
{
namespace World
{
	class Entity;

	//---------------------------------------------------------------------------------
	// A Cell represents a container for each tile in a Zone.
	//---------------------------------------------------------------------------------
	class Cell
	{
	private:
		uint32_t	m_cellID; // cellID in the Zone's m_cellMap, mainly used for comparison with cells
		int			m_cellX;
		int			m_cellY;

		std::vector<Entity*> m_entitiesHere;

	public:
		Cell(uint32_t id, int x, int y) : m_cellID(id), m_cellX(x), m_cellY(y) {}
		void Update(uint32_t diff);

		bool AddEntityHere(Entity* e);
		bool RemoveEntityHere(uint64_t entityGUID);

		const uint64_t	GetCellID() const { return m_cellID; }
		const int		GetCellX() const { return m_cellX; }
		const int		GetCellY() const { return m_cellY; }
	};

}
}