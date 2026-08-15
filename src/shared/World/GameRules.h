#pragma once

namespace NECRO
{
enum class IsoDirection
{
	WEST = 6,
	NORTH_WEST = 5,
	NORTH = 4,
	NORTH_EAST = 3,
	EAST = 2,
	SOUTH_EAST = 1,
	SOUTH = 0,
	SOUTH_WEST = 7
};

// Cells
inline constexpr int CELL_WIDTH = 64;
inline constexpr int CELL_HEIGHT = 32;

inline constexpr int HALF_CELL_WIDTH = 32;
inline constexpr int HALF_CELL_HEIGHT = 16;

// Z Pos
inline constexpr int	LAYER_Z_COEFFICIENT = 100; // A layer counts as 100 Z pos unit for entities
inline constexpr float	PLAYER_CONST_Z_POS = 0.01f; // a constant added to the player's zPos when modified

// Packet Limit
inline constexpr float	PLAYER_MOVEMENT_UPDATE_INTERVAL_SECONDS = 0.1f; // Max amount of update pos packet (50ms). The server needs to check if this is being respected by the client or if a comprimised client is trying to spam
}