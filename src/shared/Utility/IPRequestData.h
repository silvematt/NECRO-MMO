#pragma once

#include <chrono>

namespace NECRO
{
// The maximum the m_ipRequestMap can grow
inline constexpr int IP_REQUEST_MAP_MAX_SIZE = 1000000;

// IP-based spam prevention <ip, tries>
// This simply limits the number of connections (not attempts) an IP can perform within a given window
struct IPRequestData
{
	uint32_t tries;
};

// IP-Request spam prevention
// Usage:
//	std::unordered_map<uint32_t, IPRequestData> m_ipRequestMap;
}
