#pragma once

namespace NECRO
{
namespace World
{
	inline constexpr int CELL_WIDTH = 64;
	inline constexpr int CELL_HEIGHT = 32;

	inline constexpr int HALF_CELL_WIDTH = 32;
	inline constexpr int HALF_CELL_HEIGHT = 16;

	//---------------------------------------------------------------------------------
	// A Cell represents a container for each tile in a Map.
	//---------------------------------------------------------------------------------
	class Cell
	{
	private:
		int m_cellX;
		int m_cellY;
	};

}
}