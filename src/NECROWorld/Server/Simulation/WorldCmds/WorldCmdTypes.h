#pragma once

namespace NECRO
{
namespace World
{
	class PlayerEntity;

	struct PlayerSpawnCmdResult
	{
		bool			success = false;

		uint64_t		guid = 0;
		uint32_t		mapID = 0;
		float_t			posX = 0.0f;
		float_t			posY = 0.0f;
		float_t			posZ = 0.0f;
	};

	struct PlayerDespawnCmdResult
	{
		bool			success = false;
	};
}
}
