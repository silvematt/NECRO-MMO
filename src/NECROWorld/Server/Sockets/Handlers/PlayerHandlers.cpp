#include "WorldSession.h"

namespace NECRO
{
namespace World
{
	bool WorldSession::Handle_SPacketEnterWorld()
	{
		// Fixed size packet
		if (m_currentDecryptedPacket.GetActiveSize() != sizeof(SPacketEnterWorld))
			return false;

		m_lastActivity = std::chrono::steady_clock::now();

		SPacketEnterWorld* pckt = reinterpret_cast<SPacketEnterWorld*>(m_currentDecryptedPacket.GetReadPointer());
		return true;
	}
}
}