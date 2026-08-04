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
		std::atomic<uint64_t> m_nextGUID{ 1 }; // 0 is invalid - 1 is the first GUID that will be used for an entity

	public:
		static GUIDManager& Instance()
		{
			static GUIDManager manager;
			return manager;
		}

		// ----------------------------------------------------------------------------------------
		// Increments the GUID counter and returns an unique GUID.
		// ----------------------------------------------------------------------------------------
		uint64_t GetNextGUID()
		{
			return m_nextGUID.fetch_add(1, std::memory_order_relaxed);
		}
	};
}
}
