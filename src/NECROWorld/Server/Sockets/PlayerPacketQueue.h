#pragma once

#include <mutex>
#include <vector>
#include <atomic>

#include "Packet.h"

namespace NECRO
{
namespace World
{
	inline constexpr size_t PLAYER_PACKET_QUEUE_MAX_SIZE = 1000;

	// -----------------------------------------------------------------------------------------------------------------------------------------
	// An object that is shared between the WorldSession and PlayerEntity to allow the PlayerEntity to hand packets to the WorldSession without
	// any "hard link". This Queue is co-owned by the WorldSession and the PlayerEntity via shared_ptrs. 
	// 
	// It only contains packets and things that can be cleaned up by any of the two threads without causing issues.
	// -----------------------------------------------------------------------------------------------------------------------------------------
	class PlayerPacketQueue
	{
	private:
		std::mutex			m_mutex;

		// Guarded by mutex
		std::vector<Packet> m_queue;

	public:
		bool TryEnqueue(Packet&& p)
		{
			std::lock_guard lock(m_mutex);

			if (m_queue.size() >= PLAYER_PACKET_QUEUE_MAX_SIZE)
				return false;

			m_queue.push_back(std::move(p));
			return true;
		}

		// Swaps the queue with the vector passed as parameter
		void DrainQueue(std::vector<Packet>& out)
		{
			std::lock_guard lock(m_mutex);
			m_queue.swap(out);
			m_queue.clear();
		}
	};
}
}
