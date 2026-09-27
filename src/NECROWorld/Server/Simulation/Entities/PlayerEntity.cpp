#include "PlayerEntity.h"
#include "Zone.h"

namespace NECRO
{
namespace World
{
	void PlayerEntity::Update(uint32_t diff)
	{
		// Call base class update
		Entity::Update(diff);

	}

	void PlayerEntity::OnBeingAddedToZone()
	{
		Entity::OnBeingAddedToZone();

		// Everybody in the zone learns about us, and we learn about everybody.
		// Our packets are held in our PlayerPacketQueue until the WorldSession is IN_WORLD, so the client always gets them after the ENTER_WORLD response
		Packet mySpawn = BuildSpawnPacket();
		m_currentZone->ForEachPlayer([this, &mySpawn](PlayerEntity* other)
			{
				if (other == this)
					return;

				other->SendPacket(Packet(mySpawn));
				SendPacket(other->BuildSpawnPacket());
			});
	}

	void PlayerEntity::OnBeingRemovedFromZone()
	{
		Entity::OnBeingRemovedFromZone();

		// Let the other players in the zone know we're gone
		Packet despawn;
		despawn << static_cast<uint16_t>(PacketIDs::ENTITY_DESPAWN);
		despawn << static_cast<uint64_t>(m_guid);

		m_currentZone->ForEachPlayer([this, &despawn](PlayerEntity* other)
			{
				if (other != this)
					other->SendPacket(Packet(despawn));
			});
	}

	void PlayerEntity::OnCellTransferFails()
	{
		Entity::OnCellTransferFails();

		// Warn the client that this player was snapped back in a failed transfer-cell operation
        SendMovementCorrection(0);
	}

#pragma region Msgs
    bool PlayerEntity::SendPacket(Packet&& p)
    {
        if (!m_playerPacketQueue->TryEnqueue(std::move(p)))
        {
            LOG_WARNING("Could not deliver a packet to PlayerEntity GUID: '{}'.", m_guid);
            return false;
        }

        return true;
    }

    Packet PlayerEntity::BuildSpawnPacket() const
    {
        Packet p;
        p << static_cast<uint16_t>(PacketIDs::ENTITY_SPAWN);
        p << static_cast<uint64_t>(m_guid);
        p << static_cast<uint8_t>(m_type);
        p << static_cast<float_t>(m_posX);
        p << static_cast<float_t>(m_posY);
        p << static_cast<float_t>(m_posZ);
        p << static_cast<uint8_t>(m_isoDirection);
        p << static_cast<uint8_t>(m_characterData->characterName.length());
        p << m_characterData->characterName;
        return p;
    }

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
