#pragma once
#include <chrono>

namespace NECRO
{
// The maximum the m_ipRequestMap can grow
inline constexpr int IP_REQUEST_MAP_MAX_SIZE = 100000;

// IP-based spam prevention <ip, last attempt>
struct IPRequestData
{
	std::chrono::steady_clock::time_point lastUpdate;
	size_t tries;
};

// IP-Request spam prevention
// Usage:
//	std::unordered_map<std::string, IPRequestData> m_ipRequestMap;
}
