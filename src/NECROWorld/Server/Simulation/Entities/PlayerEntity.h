#pragma once

#include <memory>

#include "Entity.h"
#include "CharacterData.h"
#include "WorldSession.h"

namespace NECRO
{
namespace World
{
	class PlayerEntity : public Entity
	{
	private:
		std::unique_ptr<CharacterData>		m_characterData;
		std::shared_ptr<PlayerPacketQueue>	m_playerPacketQueue;

		// Movement Epoch/Ack
		uint32_t m_lastCorrectionID = 0;

	public:
		PlayerEntity(uint64_t guid, CharacterData charData, std::shared_ptr<PlayerPacketQueue> playerPQueue) : Entity(guid, EntityType::PLAYER_ENTITY), m_playerPacketQueue(std::move(playerPQueue)) // playerPQueue is a shared_ptr, not the resource itself
		{
			m_characterData = std::make_unique<CharacterData>(charData);

			m_posX = m_characterData->pos_x;
			m_posY = m_characterData->pos_y;
			m_posZ = m_characterData->pos_z;
		}

		void Update(uint32_t diff) override;

		const uint32_t GetLastCorrectionID() const
		{
			return m_lastCorrectionID;
		}

		const uint32_t GetCharID() const
		{
			return m_characterData->id;
		}
		
		Zone* GetCurrentZone() const
		{
			return m_currentZone;
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

		virtual void OnCellTransferFails() override;

#pragma region Msgs
		bool SendMovementCorrection(uint32_t rejectedSeq);
#pragma endregion
	};
}
}
