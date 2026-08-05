#pragma once

#include <cstdint>
#include <atomic>

namespace NECRO
{
namespace World
{
	// ----------------------------------------------------------------------------------------
	// Manages GUID, only source where GUID will be generated.
	// ----------------------------------------------------------------------------------------
	class GUIDManager
	{
	private:
		static inline std::atomic<uint64_t> s_nextGUID{ 1 }; // 0 is invalid - 1 is the first GUID that will be used for an entity

	public:

		// ----------------------------------------------------------------------------------------
		// Increments the GUID counter and returns an unique GUID.
		// ----------------------------------------------------------------------------------------
		static uint64_t GetNextGUID()
		{
			return s_nextGUID.fetch_add(1, std::memory_order_relaxed);
		}
	};
}
}
