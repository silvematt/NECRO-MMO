#pragma once

#include "Entity.h"

namespace NECRO
{
namespace World
{
	class PlayerEntity : public Entity
	{
	public:
		PlayerEntity(uint64_t guid) : Entity(guid, EntityType::PLAYER_ENTITY)
		{
		}

		PlayerEntity(uint64_t guid, float posX, float posY) : Entity(guid, EntityType::PLAYER_ENTITY, posX, posY)
		{
		}
	};
}
}
