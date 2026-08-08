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
		PlayerEntity(uint64_t guid, CharacterData charData) : Entity(guid, EntityType::PLAYER_ENTITY)
		{
			m_characterData = std::make_unique<CharacterData>(charData);
			m_posX = m_characterData->pos_x;
			m_posY = m_characterData->pos_y;
		}

		void Update(uint32_t diff) override;

		const uint32_t GetCharID() const
		{
			return m_characterData->id;
		}
		
		Map* GetCurrentMap() const
		{
			return m_currentMap;
		}
	};
}
}
