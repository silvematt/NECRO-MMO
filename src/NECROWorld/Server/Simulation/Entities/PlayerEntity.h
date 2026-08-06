#pragma once

#include <memory>

#include "Entity.h"
#include "CharacterData.h"


namespace NECRO
{
namespace World
{
	class PlayerEntity : public Entity
	{
	private:
		std::unique_ptr<CharacterData> m_characterData;

	public:
		PlayerEntity(uint64_t guid, std::shared_ptr<CharacterData> charData) : Entity(guid, EntityType::PLAYER_ENTITY)
		{
			m_characterData = std::make_unique<CharacterData>(*charData);
		}
	};
}
}
