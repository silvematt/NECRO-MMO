#include "PlayerEntity.h"

namespace NECRO
{
namespace World
{
	void PlayerEntity::Update(uint32_t diff)
	{
		// Call base class update
		Entity::Update(diff);

	}

	void PlayerEntity::OnCellTransferFails()
	{
		Entity::OnCellTransferFails();

		// Warn the client that this player was snapped back in a failed transfer-cell operation
        SendMovementCorrection(0);
	}

#pragma region Msgs
    bool PlayerEntity::SendMovementCorrection(uint32_t rejectedSeq)
    {
        // Do NOT commit the new correctionID until the packet is actually queued.
        uint32_t nextCorrectionID = m_lastCorrectionID + 1;

        Packet p;
        p << static_cast<uint16_t>(PacketIDs::PLAYER_MOVEMENT_CORRECTION);
        p << static_cast<uint32_t>(nextCorrectionID);
        p << static_cast<uint32_t>(rejectedSeq);
        p << static_cast<float_t>(m_posX);
        p << static_cast<float_t>(m_posY);
        p << static_cast<float_t>(m_posZ);
        p << static_cast<uint8_t>(m_isoDirection);

        if (!m_playerPacketQueue->TryEnqueue(std::move(p)))
        {
            // The correction never left. Keep the current epoch so the client's packets are still accepted.
            LOG_WARNING("Could not deliver a movement correction to PlayerEntity GUID: '{}'.", m_guid);
            return false;
        }

        m_lastCorrectionID = nextCorrectionID;
        LOG_DEBUG("Correction sent ID:'{}' (rejectedSeq '{}') to GUID '{}'", m_lastCorrectionID, rejectedSeq, m_guid);
        return true;
    }
#pragma endregion
}
}
