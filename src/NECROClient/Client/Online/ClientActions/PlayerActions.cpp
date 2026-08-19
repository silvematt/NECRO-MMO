#include "WorldManager.h"
#include "NECROEngine.h"
#include "Player.h"

// --------------------------------------------------------------------------------------------
// Client Actions are Online-related actions the client performs.
// --------------------------------------------------------------------------------------------

namespace NECRO
{
namespace Client
{
	void WorldManager::SendPlayerMovementUpdate()
	{
		// Attempt to update - maybe some extra check here?
		Player* p = engine.GetGame().GetCurPlayer();

		if (p)
		{
			// Nothing to update?
			if (!p->m_isMovementDirty)
				return;

			Packet pckt;
			pckt << static_cast<uint16_t>(NECRO::World::PacketIDs::PLAYER_MOVEMENT_UPDATE);

			pckt << static_cast<uint32_t>(m_data.m_currentMovSeq);
			pckt << static_cast<uint32_t>(m_data.m_curretnAckedCorrectionID);

			pckt << static_cast<float_t>(p->m_pos.x);
			pckt << static_cast<float_t>(p->m_pos.y);
			pckt << static_cast<float_t>(p->m_zPos);
			pckt << static_cast<uint8_t>(p->m_isoDirection);

			NetworkMessage encrypted(std::move(pckt));
			if (encrypted.AESEncrypt(engine.GetAuthManager().GetData().sessionKey.data(), engine.GetAuthManager().GetData().iv, nullptr, 0) < 0)
			{
				LOG_ERROR("Failed to encrypt packet.");
				return;
			}

			m_data.m_currentMovSeq++;

			NetworkMessage m(std::move(encrypted));
			engine.GetWorldManager().QueuePacket(NetworkMessage(std::move(m)));

			p->m_isMovementDirty = false;
		}
		else
		{
			LOG_WARNING("Called SendPlayerMovementUpdate but GetCurPlayer() returned null!");
		}
	}
}
}
