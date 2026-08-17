#include "PlayerEntity.h"
#include <boost/asio.hpp>

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
        // If we bumped it and the send failed, the client would keep acking the old ID forever and every
        // one of its movement packets would be dropped as stale: the player would be frozen for good.
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
            // The correction never left. Keep the current epoch so the client's packets are still accepted:
            // it stays desynced, but its next movement packet gets rejected and we retry the correction.
            LOG_WARNING("Could not deliver a movement correction to PlayerEntity GUID: '{}'.", m_guid);
            return false;
        }

        m_lastCorrectionID = nextCorrectionID;
        LOG_DEBUG("Correction sent ID:'{}' (rejectedSeq '{}') to GUID '{}'", m_lastCorrectionID, m_guid);
        return true;
    }
#pragma endregion
}
}
