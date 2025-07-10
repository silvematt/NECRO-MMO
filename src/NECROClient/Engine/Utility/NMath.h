#ifndef NECROMATH_H
#define NECROMATH_H

#include "Cell.h"

class NMath
{
public:
	static void IsoToCart(int screenx, int screeny, int& mapx, int& mapy);
	static void CartToIso(int mapx, int mapy, int& screenx, int& screeny);

	static void IsoToCart(float screenx, float screeny, float& mapx, float& mapy);
	static void CartToIso(float mapx, float mapy, float& screenx, float& screeny);
};

//-------------------------------------------------------------------------------------------
// Converting Cartesian coordinates to Isometric coordinates.
// 
// !The conversion is in map units!
// 
// Assuming the map is 16x16, When mapx and mapy are both 7, you get the tile at the center 
// of the rhombus.
// Also assuming you're looking at the isometric map on the screen with the Y direction 
// representing the Y direction of the 2D screen (up/down):
// 
// When mapx and mapy are both 0, you get the tile at the top of the rhombus.
// When mapx and mapy are both 1, you move one tile down on the Y axis (down because we're counting from the top of the screen in the Y direction, imagining the mouse cursor moving only down).
// 
// If mapx and mapy are both 0.5, the function returns the isometric coordinates between the two cells,
// that is the point where the bottom of the first cell and the top of the second cell meet.
// 
// We can keep things orthographic to do position, collision etc while using floats.
// x = 64.0f, y = 0.0f
// 
// To draw simply call 
//		CartToIso(x/TILE_WIDTH, y/TILE_HEIGHT, screenx, screeny);
//-------------------------------------------------------------------------------------------

inline void NMath::CartToIso(int mapx, int mapy, int& screenx, int& screeny)
{
	screenx = (mapx - mapy) * HALF_CELL_WIDTH;
	screeny = (mapx + mapy) * HALF_CELL_HEIGHT;
}

inline void NMath::IsoToCart(int screenx, int screeny , int& mapx, int& mapy)
{
	// Convert to calculate in floating point
	float screenxf = static_cast<float>(screenx);
	float screenyf = static_cast<float>(screeny);

	// Decimal part is truncated
	mapx = static_cast<int>(screenxf / CELL_WIDTH + screenyf / CELL_HEIGHT);
	mapy = static_cast<int>(screenyf / CELL_HEIGHT - screenxf / CELL_WIDTH);
}

inline void NMath::CartToIso(float mapx, float mapy, float& screenx, float& screeny)
{
	screenx = (mapx - mapy) * HALF_CELL_WIDTH;
	screeny = (mapx + mapy) * HALF_CELL_HEIGHT;
}

inline void NMath::IsoToCart(float screenx, float screeny, float& mapx, float& mapy)
{
	mapx = (screenx / CELL_WIDTH + screeny / CELL_HEIGHT);
	mapy = (screeny / CELL_HEIGHT - screenx / CELL_WIDTH);
}


#endif
