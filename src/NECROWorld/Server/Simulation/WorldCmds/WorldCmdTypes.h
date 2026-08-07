#pragma once

namespace NECRO
{
namespace World
{
	struct PlayerSpawnCmdResult
	{
		bool success;

		uint64_t     guid = 0;
		uint32_t     mapID = 0;
		float        posX = 0.0f;
		float        posY = 0.0f;
	};
}
}