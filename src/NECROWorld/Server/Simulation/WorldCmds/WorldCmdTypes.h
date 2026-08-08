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
		PlayerEntity*	playerPtr = nullptr;
		uint32_t		mapID = 0;
		float			posX = 0.0f;
		float			posY = 0.0f;
	};
}
}