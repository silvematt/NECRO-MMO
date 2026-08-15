#include "WorldManager.h"
#include "NECROEngine.h"
#include "Player.h"

namespace NECRO
{
namespace Client
{
	void WorldManager::SendPlayerMovementUpdate()
	{
		// Check if we should send

		// Attempt to update
		Player* p = engine.GetGame().GetCurPlayer();

		if (p)
		{
			// Nothing to update?
			if (!p->m_isMovementDirty)
				return;

			Packet pckt;
			pckt << static_cast<uint16_t>(NECRO::World::PacketIDs::PLAYER_MOVEMENT_UPDATE);
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
