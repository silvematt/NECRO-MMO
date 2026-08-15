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
		float_t			posX = 0.0f;
		float_t			posY = 0.0f;
		float_t			posZ = 0.0f;
	};

	struct PlayerDespawnCmdResult
	{
		bool			success = false;
	};

	struct PlayerMovementUpdateCmdResult
	{
		bool accepted = false;

		uint32_t pcktSeq = 0;

		float_t newPosX = 0.0f;
		float_t newPosY = 0.0f;
		float_t newPosZ = 0.0f;
		uint8_t newIsoDirection = 0;
	};
}
}
