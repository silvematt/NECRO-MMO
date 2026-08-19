#include "WorldSession.h"
#include "WorldCodes.h"
#include "NECROEngine.h"
#include "Console.h"
#include "Player.h"

namespace NECRO
{
namespace Client
{
	bool WorldSession::Handle_PlayerMovementCorrection()
	{
		if (!IsOpen())
			return false;

		LOG_DEBUG("Handle_PlayerMovementCorrection!");

		Console& c = engine.GetConsole();
		auto& onlineData = engine.GetWorldManager().GetData();
		Player* p = engine.GetGame().GetCurPlayer();

		if (p)
		{
			p->m_isMovementDirty = true;
			NECRO::World::CPacketPlayerMovementCorrection* pckt = reinterpret_cast<NECRO::World::CPacketPlayerMovementCorrection*>(m_currentDecryptedPacket.GetReadPointer());

			p->ExecuteMovementCorrection(pckt);

			// Update correction id!!!
			onlineData.m_curretnAckedCorrectionID = pckt->correctionID;

			// Ack to the server.
			engine.GetWorldManager().SendPlayerMovementUpdate();
		}
		else
		{
			LOG_ERROR("Handle_PlayerMovementCorrection - engine.GetGame().GetCurPlayer returned null pointer!");
			return false;
		}


		return true;
	}
}
}
