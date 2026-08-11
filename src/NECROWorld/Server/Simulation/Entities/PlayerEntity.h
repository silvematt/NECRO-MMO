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
			m_posZ = m_characterData->pos_z
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

		// Updates m_characterData to the current in memory values, usually done before saving on the database
		void UpdateCharacterData()
		{
			// TODO Update all data
			m_characterData->pos_x = m_posX;
			m_characterData->pos_y = m_posY;
			m_characterData->pos_z = m_posZ;
		}

		const CharacterData* GetCharacterData()
		{
			UpdateCharacterData(); // get character data also updates the data to return
			return m_characterData.get();
		}
	};
}
}
